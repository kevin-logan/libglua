#pragma once

// NOTE: Do not include this, include glua/backends/lua.hpp instead, the include order is carefully
// crafted to separate declarations and dependent definitions

namespace glua::lua {
template <decays_to<std::string> T>
result<void> converter<T>::push_to_lua(lua_State* lua, const std::string& v)
{
    lua_pushstring(lua, v.data());
    return {};
}

template <decays_to<std::string> T>
result<std::string> converter<T>::from_lua(lua_State* lua, int i)
{
    const char* str = lua_tostring(lua, i);
    if (str) {
        return std::string { str };
    } else {
        return unexpected("lua value could not be converted to string");
    }
}

template <decays_to<std::string_view> T>
result<void> converter<T>::push_to_lua(lua_State* lua, std::string_view v)
{
    // needs to be null terminated, so we need a copy
    std::string copy { v };
    lua_pushstring(lua, copy.data());
    return {};
}

template <decays_to<const char*> T>
result<void> converter<T>::push_to_lua(lua_State* lua, const char* v)
{
    lua_pushstring(lua, v);
    return {};
}

template <typename T>
result<void> converter<std::vector<T>>::push_to_lua(lua_State* lua, std::vector<T>&& v)
{
    auto fallback = lua_gettop(lua);
    lua_newtable(lua);

    auto length = v.size();
    for (std::size_t i = 0; i < length; ++i) {
        lua_pushinteger(lua, static_cast<lua_Integer>(i + 1)); // 1 based in lua
        auto inner_result = lua::push_to_lua(lua, std::move(v[i]));
        if (!inner_result.has_value()) {
            lua_settop(lua, fallback);
            return unexpected(std::move(inner_result).error());
        }

        lua_settable(lua, -3);
    }

    return {};
}

template <typename T>
result<void> converter<std::vector<T>>::push_to_lua(lua_State* lua, const std::vector<T>& v)
{
    auto fallback = lua_gettop(lua);
    lua_newtable(lua);

    auto length = v.size();
    for (std::size_t i = 0; i < length; ++i) {
        lua_pushinteger(lua, static_cast<lua_Integer>(i + 1)); // 1 based in lua
        auto inner_result = lua::push_to_lua(lua, v[i]);
        if (!inner_result.has_value()) {
            lua_settop(lua, fallback);
            return unexpected(std::move(inner_result).error());
        }

        lua_settable(lua, -3);
    }

    return {};
}

template <typename T>
result<std::vector<T>> converter<std::vector<T>>::from_lua(lua_State* lua, int i)
{
    auto fallback = lua_gettop(lua);
    auto absolute_index = make_absolute(lua, i);

    if (lua_istable(lua, absolute_index)) {
        std::size_t len = lua_objlen(lua, absolute_index);
        std::vector<T> result;
        result.reserve(len);

        for (std::size_t i = 0; i < len; ++i) {
            lua_pushinteger(lua, static_cast<lua_Integer>(i + 1)); // 1 based in lua
            lua_gettable(lua, absolute_index);
            auto inner_result = lua::from_lua<T>(lua, -1);
            if (inner_result.has_value()) {
                result.push_back(std::move(inner_result).value());
            } else {
                lua_settop(lua, fallback);
                return unexpected(std::move(inner_result).error());
            }
        }

        return result;
    } else {
        return unexpected("Could not convert non-table to vector");
    }
}

template <decays_to_vector T>
result<void> converter<T>::push_to_lua(lua_State* lua, T v)
{
    return converter<std::decay_t<T>>::push_to_lua(lua, v);
}

template <typename T>
result<void> converter<std::unordered_map<std::string, T>>::push_to_lua(lua_State* lua, std::unordered_map<std::string, T>&& v)
{
    auto fallback = lua_gettop(lua);
    lua_newtable(lua);

    for (auto& [key, value] : v) {
        lua_pushstring(lua, key.data());
        auto inner_result = lua::push_to_lua(lua, std::move(value));
        if (!inner_result.has_value()) {
            lua_settop(lua, fallback);
            return unexpected(std::move(inner_result).error());
        }

        lua_settable(lua, -3);
    }

    return {};
}

template <typename T>
result<void> converter<std::unordered_map<std::string, T>>::push_to_lua(lua_State* lua, const std::unordered_map<std::string, T>& v)
{
    auto fallback = lua_gettop(lua);
    lua_newtable(lua);

    for (const auto& [key, value] : v) {
        lua_pushstring(lua, key.data());
        auto inner_result = lua::push_to_lua(lua, value);
        if (!inner_result.has_value()) {
            lua_settop(lua, fallback);
            return unexpected(std::move(inner_result).error());
        }

        lua_settable(lua, -3);
    }

    return {};
}

template <typename T>
result<std::unordered_map<std::string, T>> converter<std::unordered_map<std::string, T>>::from_lua(lua_State* lua, int i)
{
    auto fallback = lua_gettop(lua);
    auto absolute_index = make_absolute(lua, i);

    if (lua_istable(lua, absolute_index)) {
        std::unordered_map<std::string, T> result;

        lua_pushnil(lua); // first key is null for iteration
        while (lua_next(lua, absolute_index) != 0) {
            // key at -2, value at -1
            // copy key as we'll convert to string and that might interfere with lua_next
            lua_pushvalue(lua, -2); // now key -3, value -2, key -1
            auto inner_result = lua::from_lua<std::string>(lua, -1).and_then([&](auto key) {
                return lua::from_lua<T>(lua, -2).transform([&](auto value) {
                    result.emplace(std::move(key), std::move(value));
                });
            });
            if (!inner_result.has_value()) {
                lua_settop(lua, fallback);
                return unexpected(std::move(inner_result).error());
            }

            // success so pop copy of key and the value
            lua_pop(lua, 2);
        }

        return result;
    } else {
        return unexpected("Could not convert non-table to unordered_map");
    }
}

template <decays_to_unordered_map T>
result<void> converter<T>::push_to_lua(lua_State* lua, T v)
{
    return converter<std::decay_t<T>>::push_to_lua(lua, v);
}

inline result<void> converter<int8_t>::push_to_lua(lua_State* lua, int8_t v)
{
    lua_pushinteger(lua, static_cast<lua_Integer>(v));
    return {};
}

inline result<int8_t> converter<int8_t>::from_lua(lua_State* lua, int i)
{
    return static_cast<int8_t>(lua_tointeger(lua, i));
}

inline result<void> converter<uint8_t>::push_to_lua(lua_State* lua, uint8_t v)
{
    lua_pushinteger(lua, static_cast<lua_Integer>(v));
    return {};
}

inline result<uint8_t> converter<uint8_t>::from_lua(lua_State* lua, int i)
{
    return static_cast<uint8_t>(lua_tointeger(lua, i));
}

inline result<void> converter<int16_t>::push_to_lua(lua_State* lua, int16_t v)
{
    lua_pushinteger(lua, static_cast<lua_Integer>(v));
    return {};
}

inline result<int16_t> converter<int16_t>::from_lua(lua_State* lua, int i)
{
    return static_cast<int16_t>(lua_tointeger(lua, i));
}

inline result<void> converter<uint16_t>::push_to_lua(lua_State* lua, uint16_t v)
{
    lua_pushinteger(lua, static_cast<lua_Integer>(v));
    return {};
}

inline result<uint16_t> converter<uint16_t>::from_lua(lua_State* lua, int i)
{
    return static_cast<uint16_t>(lua_tointeger(lua, i));
}

inline result<void> converter<int32_t>::push_to_lua(lua_State* lua, int32_t v)
{
    lua_pushinteger(lua, static_cast<lua_Integer>(v));
    return {};
}

inline result<int32_t> converter<int32_t>::from_lua(lua_State* lua, int i)
{
    return static_cast<int32_t>(lua_tointeger(lua, i));
}

inline result<void> converter<uint32_t>::push_to_lua(lua_State* lua, uint32_t v)
{
    lua_pushinteger(lua, static_cast<lua_Integer>(v));
    return {};
}

inline result<uint32_t> converter<uint32_t>::from_lua(lua_State* lua, int i)
{
    return static_cast<uint32_t>(lua_tointeger(lua, i));
}

inline result<void> converter<int64_t>::push_to_lua(lua_State* lua, int64_t v)
{
    lua_pushinteger(lua, static_cast<lua_Integer>(v));
    return {};
}

inline result<int64_t> converter<int64_t>::from_lua(lua_State* lua, int i)
{
    return static_cast<int64_t>(lua_tointeger(lua, i));
}

inline result<void> converter<uint64_t>::push_to_lua(lua_State* lua, uint64_t v)
{
    lua_pushinteger(lua, static_cast<lua_Integer>(v));
    return {};
}

inline result<uint64_t> converter<uint64_t>::from_lua(lua_State* lua, int i)
{
    return static_cast<uint64_t>(lua_tointeger(lua, i));
}

inline result<void> converter<float>::push_to_lua(lua_State* lua, float v)
{
    lua_pushnumber(lua, static_cast<lua_Number>(v));
    return {};
}

inline result<float> converter<float>::from_lua(lua_State* lua, int i)
{
    return static_cast<float>(lua_tonumber(lua, i));
}

inline result<void> converter<double>::push_to_lua(lua_State* lua, double v)
{
    lua_pushnumber(lua, static_cast<lua_Number>(v));
    return {};
}

inline result<double> converter<double>::from_lua(lua_State* lua, int i)
{
    return static_cast<double>(lua_tonumber(lua, i));
}

inline result<void> converter<bool>::push_to_lua(lua_State* lua, bool v)
{
    lua_pushboolean(lua, static_cast<int>(v));
    return {};
}

inline result<bool> converter<bool>::from_lua(lua_State* lua, int i)
{
    return lua_toboolean(lua, i) != 0;
}

template <registered_class T>
result<void> converter<T*>::push_to_lua(lua_State* lua, T* v)
{
    return class_registration_impl<T>::push_wrapped_object(lua, v);
}

template <registered_class T>
result<T*> converter<T*>::from_lua(lua_State* lua, int i) { return class_registration_impl<T>::unwrap_object(lua, i); }

template <registered_class T>
result<void> converter<const T*>::push_to_lua(lua_State* lua, const T* v)
{
    return class_registration_impl<T>::push_wrapped_object(lua, v);
}

template <registered_class T>
result<const T*> converter<const T*>::from_lua(lua_State* lua, int i) { return class_registration_impl<T>::unwrap_object_const(lua, i); }

template <registered_class T>
result<void> converter<std::unique_ptr<T>>::push_to_lua(lua_State* lua, std::unique_ptr<T> v)
{
    return class_registration_impl<T>::push_wrapped_object(lua, std::move(v));
}

template <registered_class T>
result<void> converter<std::unique_ptr<const T>>::push_to_lua(lua_State* lua, std::unique_ptr<const T> v)
{
    return class_registration_impl<T>::push_wrapped_object(lua, std::move(v));
}

template <registered_class T>
result<void> converter<T&>::push_to_lua(lua_State* lua, T& v)
{
    return class_registration_impl<T>::push_wrapped_object(lua, &v);
}

template <registered_class T>
result<std::reference_wrapper<T>> converter<T&>::from_lua(lua_State* lua, int i)
{
    return class_registration_impl<T>::unwrap_object(lua, i).transform([](auto* ptr) { return std::ref(*ptr); });
}

template <registered_class T>
result<void> converter<const T&>::push_to_lua(lua_State* lua, const T& v)
{
    return class_registration_impl<T>::push_wrapped_object(lua, &v);
}

template <registered_class T>
result<std::reference_wrapper<const T>> converter<const T&>::from_lua(lua_State* lua, int i)
{
    return class_registration_impl<T>::unwrap_object_const(lua, i).transform([](const T* ptr) { return std::cref(*ptr); });
}

template <registered_class T>
result<void> converter<std::reference_wrapper<T>>::push_to_lua(lua_State* lua, std::reference_wrapper<T> v)
{
    return class_registration_impl<T>::push_wrapped_object(lua, &v.get());
}

template <registered_class T>
result<std::reference_wrapper<T>> converter<std::reference_wrapper<T>>::from_lua(lua_State* lua, int i)
{
    return class_registration_impl<T>::unwrap_object(lua, i).transform([](auto* ptr) { return std::ref(*ptr); });
}

template <registered_class T>
result<void> converter<std::reference_wrapper<const T>>::push_to_lua(lua_State* lua, std::reference_wrapper<const T> v)
{
    return class_registration_impl<T>::push_wrapped_object(lua, &v.get());
}

template <registered_class T>
result<std::reference_wrapper<const T>> converter<std::reference_wrapper<const T>>::from_lua(lua_State* lua, int i)
{
    return class_registration_impl<T>::unwrap_object_const(lua, i).transform([](const T* ptr) { return std::cref(*ptr); });
}

inline result<void> converter<any>::push_to_lua(lua_State* lua, const any& v)
{
    auto* lua_any = dynamic_cast<any_lua_impl*>(v.impl_.get());
    if (lua_any) {
        return lua_any->push_to_lua(lua);
    } else {
        return unexpected("Received invalid any, perhaps from another backend");
    }
}

inline result<any> converter<any>::from_lua(lua_State* lua, int i)
{
    auto absolute_index = make_absolute(lua, i);
    auto type = lua_type(lua, absolute_index);

    return [&]() -> result<std::unique_ptr<any_impl>> {
        switch (type) {
        case LUA_TBOOLEAN:
            return lua::from_lua<bool>(lua, i).transform([&](auto value) -> std::unique_ptr<any_impl> {
                return std::make_unique<any_basic_impl>(value);
            });
        case LUA_TNUMBER:
            return lua::from_lua<double>(lua, i).transform([&](auto value) -> std::unique_ptr<any_impl> {
                return std::make_unique<any_basic_impl>(value);
            });
        case LUA_TSTRING:
            return lua::from_lua<std::string>(lua, i).transform([&](auto value) -> std::unique_ptr<any_impl> {
                return std::make_unique<any_basic_impl>(std::move(value));
            });
        case LUA_TTABLE: {
            // array or map
            auto len = lua_objlen(lua, absolute_index);
            auto treat_as_array = [&]() {
                if (len > 0) {
                    // certainly an array, has a len
                    return true;
                } else {
                    // empty array or map
                    lua_pushnil(lua);
                    if (lua_next(lua, absolute_index) != 0) {
                        // it's a map, len was 0 but there was an item anyway
                        lua_pop(lua, 2); // pop the key and value
                        return false;
                    }

                    // empty on all counts, array is easier than map
                    return true;
                }
            }();

            if (treat_as_array) {
                return lua::from_lua<std::vector<any>>(lua, i).transform([&](auto value) -> std::unique_ptr<any_impl> {
                    return std::make_unique<any_array_impl<any>>(std::move(value));
                });
            } else {
                return lua::from_lua<std::unordered_map<std::string, any>>(lua, i).transform([&](auto value) -> std::unique_ptr<any_impl> {
                    return std::make_unique<any_map_impl<any>>(std::move(value));
                });
            }
        }
        case LUA_TUSERDATA: {
            // registered_class
            auto* data = static_cast<class_registration_data_ptr*>(lua_touserdata(lua, absolute_index));
            return data->get()->make_any_(data);
        }
        }
        return unexpected("Cannot create any from invalid typed lua item");
    }()
                        .transform([&](auto any_ptr) {
                            return any { std::move(any_ptr) };
                        });
}

template <typename T, typename... Ts>
result<void> many_push_to_lua(lua_State* lua, T&& value, Ts&&... values)
{
    // pushing itself is the action we want
    // we can't do this like we did it for the spidermonkey impl, for example,
    // as the result to push_to_lua can't be put in a parameter list, call
    // order of parameters is undefined. Instead explicitly call them in order
    // and short circuit if there is an error
    return push_to_lua(lua, static_cast<T&&>(value)).and_then([&]() -> result<void> {
        // succeeded, return or call the next one
        if constexpr (sizeof...(Ts) > 0) {
            return many_push_to_lua(lua, static_cast<Ts&&>(values)...).transform_error([&](auto error) {
                lua_pop(lua, 1);
                return error;
            });
        } else {
            return {};
        }
    });
}

inline result<void> many_push_to_lua(lua_State*) { return {}; }

template <typename... Ts>
auto many_from_lua([[maybe_unused]] lua_State* lua, IndexFor<Ts>... vs)
{
    auto many_expected = std::make_tuple(from_lua<Ts>(lua, vs)...);
    return many_results_to_one(std::move(many_expected));
}
}
