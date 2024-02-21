#pragma once

#include "glua/glua.hpp"

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include <format>
#include <map>

namespace glua::lua {
template <typename T>
struct converter {
};

template <decays_to<std::string> T>
struct converter<T> {
    static result<void> push_to_lua(lua_State* lua, const std::string& v)
    {
        lua_pushstring(lua, v.data());
        return {};
    }

    static result<std::string> from_lua(lua_State* lua, lua_Integer i)
    {
        const char* str = lua_tostring(lua, i);
        if (str) {
            return std::string { str };
        } else {
            return unexpected("lua value could not be converted to string");
        }
    }
};

template <decays_to<std::string_view> T>
struct converter<T> {
    static result<void> push_to_lua(lua_State* lua, std::string_view v)
    {
        // needs to be null terminated, so we need a copy
        std::string copy { v };
        lua_pushstring(lua, copy.data());
        return {};
    }

    // from_lua not supported, can't 'point' to script bytes
};

template <>
struct converter<int8_t> {
    static result<void> push_to_lua(lua_State* lua, int8_t v)
    {
        lua_pushinteger(lua, static_cast<lua_Integer>(v));
        return {};
    }

    static result<int8_t> from_lua(lua_State* lua, lua_Integer i)
    {
        return static_cast<int8_t>(lua_tointeger(lua, i));
    }
};

template <>
struct converter<uint8_t> {
    static result<void> push_to_lua(lua_State* lua, uint8_t v)
    {
        lua_pushinteger(lua, static_cast<lua_Integer>(v));
        return {};
    }

    static result<uint8_t> from_lua(lua_State* lua, lua_Integer i)
    {
        return static_cast<uint8_t>(lua_tointeger(lua, i));
    }
};

template <>
struct converter<int16_t> {
    static result<void> push_to_lua(lua_State* lua, int16_t v)
    {
        lua_pushinteger(lua, static_cast<lua_Integer>(v));
        return {};
    }

    static result<int16_t> from_lua(lua_State* lua, lua_Integer i)
    {
        return static_cast<int16_t>(lua_tointeger(lua, i));
    }
};

template <>
struct converter<uint16_t> {
    static result<void> push_to_lua(lua_State* lua, uint16_t v)
    {
        lua_pushinteger(lua, static_cast<lua_Integer>(v));
        return {};
    }

    static result<uint16_t> from_lua(lua_State* lua, lua_Integer i)
    {
        return static_cast<uint16_t>(lua_tointeger(lua, i));
    }
};

template <>
struct converter<int32_t> {
    static result<void> push_to_lua(lua_State* lua, int32_t v)
    {
        lua_pushinteger(lua, static_cast<lua_Integer>(v));
        return {};
    }

    static result<int32_t> from_lua(lua_State* lua, lua_Integer i)
    {
        return static_cast<int32_t>(lua_tointeger(lua, i));
    }
};

template <>
struct converter<uint32_t> {
    static result<void> push_to_lua(lua_State* lua, uint32_t v)
    {
        lua_pushinteger(lua, static_cast<lua_Integer>(v));
        return {};
    }

    static result<uint32_t> from_lua(lua_State* lua, lua_Integer i)
    {
        return static_cast<uint32_t>(lua_tointeger(lua, i));
    }
};

template <>
struct converter<int64_t> {
    static result<void> push_to_lua(lua_State* lua, int64_t v)
    {
        lua_pushinteger(lua, static_cast<lua_Integer>(v));
        return {};
    }

    static result<int64_t> from_lua(lua_State* lua, lua_Integer i)
    {
        return static_cast<int64_t>(lua_tointeger(lua, i));
    }
};

template <>
struct converter<uint64_t> {
    static result<void> push_to_lua(lua_State* lua, uint64_t v)
    {
        lua_pushinteger(lua, static_cast<lua_Integer>(v));
        return {};
    }

    static result<uint64_t> from_lua(lua_State* lua, lua_Integer i)
    {
        return static_cast<uint64_t>(lua_tointeger(lua, i));
    }
};

template <>
struct converter<float> {
    static result<void> push_to_lua(lua_State* lua, float v)
    {
        lua_pushnumber(lua, static_cast<lua_Number>(v));
        return {};
    }

    static result<float> from_lua(lua_State* lua, lua_Integer i)
    {
        return static_cast<float>(lua_tonumber(lua, i));
    }
};

template <>
struct converter<double> {
    static result<void> push_to_lua(lua_State* lua, double v)
    {
        lua_pushnumber(lua, static_cast<lua_Number>(v));
        return {};
    }

    static result<double> from_lua(lua_State* lua, lua_Integer i)
    {
        return static_cast<double>(lua_tonumber(lua, i));
    }
};

template <>
struct converter<bool> {
    static result<void> push_to_lua(lua_State* lua, bool v)
    {
        lua_pushboolean(lua, static_cast<int>(v));
        return {};
    }

    static result<bool> from_lua(lua_State* lua, lua_Integer i)
    {
        return lua_toboolean(lua, i) != 0;
    }
};

template <decays_to_integral T>
struct converter<T> : converter<std::decay_t<T>> { };

template <typename T>
auto push_to_lua(lua_State* lua, T&& value)
{
    return converter<T>::push_to_lua(lua, static_cast<T&&>(value));
}

template <typename T>
auto from_lua(lua_State* lua, lua_Integer i)
{
    return converter<T>::from_lua(lua, i);
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

template <typename T>
using IndexFor = lua_Integer;

template <typename... Ts>
auto many_from_lua(lua_State* lua, IndexFor<Ts>... vs)
{
    auto many_expected = std::make_tuple(from_lua<Ts>(lua, vs)...);
    return many_results_to_one(std::move(many_expected));
}

template <typename ReturnType, typename... ArgTypes>
int call_generic_wrapped_functor(generic_functor<ReturnType, ArgTypes...>& f, lua_State* lua);

template <registered_class T>
struct class_registration_impl {
    using registration = class_registration<T>;

    struct wrap_data {
        wrap_data(T* ptr, bool owned_by_lua, bool is_mutable)
            : ptr_(ptr)
            , owned_by_lua_(owned_by_lua)
            , mutable_(is_mutable)
        {
        }

        T* ptr_;
        bool owned_by_lua_;
        bool mutable_;
    };

    static result<T*> unwrap_object(lua_State* lua, lua_Integer i)
    {
        if (lua_isuserdata(lua, i)) {
            auto* data = static_cast<wrap_data*>(lua_touserdata(lua, 1));
            if (data->mutable_) {
                return data->ptr_;
            } else {
                return unexpected("unwrap failed - attempted to extract mutable reference to const object");
            }
        } else {
            return unexpected("unwrap on non-object value");
        }
    }

    static result<const T*> unwrap_object_const(lua_State* lua, lua_Integer i)
    {
        if (lua_isuserdata(lua, i)) {
            auto* data = static_cast<wrap_data*>(lua_touserdata(lua, 1));
            return data->ptr_;
        } else {
            return unexpected("unwrap on non-object value");
        }
    }

    static result<void> push_wrapped_object(lua_State* lua, T* obj_ptr, bool owned_by_lua = false)
    {
        wrap_data* data = static_cast<wrap_data*>(lua_newuserdata(lua, sizeof(wrap_data)));
        new (data) wrap_data { obj_ptr, owned_by_lua, true };

        // set the metatable for this object
        luaL_getmetatable(lua, registration::name.data());
        lua_setmetatable(lua, -2);

        return {};
    }

    static result<void> push_wrapped_object(lua_State* lua, std::unique_ptr<T> obj_ptr)
    {
        return push_wrapped_object(lua, obj_ptr.release(), true);
    }

    static result<void> push_wrapped_object(lua_State* lua, const T* obj_ptr, bool owned_by_lua = false)
    {
        wrap_data* data = static_cast<wrap_data*>(lua_newuserdata(lua, sizeof(wrap_data)));

        // const_cast here is obviously a const violation, it means at runtime we're now responsible
        // for const checking
        new (data) wrap_data { const_cast<T*>(obj_ptr), owned_by_lua, false };

        // set the metatable for this object
        luaL_getmetatable(lua, registration::name.data());
        lua_setmetatable(lua, -2);

        return {};
    }

    static result<void> push_wrapped_object(lua_State* lua, std::unique_ptr<const T> obj_ptr)
    {
        return push_wrapped_object(lua, obj_ptr.release(), true);
    }

    static int constructor(lua_State* lua)
    {
        return call_generic_wrapped_functor(*registration::constructor, lua);
    }

    static int destructor(lua_State* lua)
    {
        auto* data = static_cast<wrap_data*>(lua_touserdata(lua, 1));
        if (data->owned_by_lua_) {
            // reacquire and destruct
            auto owned = std::unique_ptr<T>(data->ptr_);
        }

        // then we need to destroy the data
        std::destroy_at(data);

        return 0;
    }

    template <std::size_t MethodIndex>
    static int method_call(lua_State* lua)
    {
        return call_generic_wrapped_functor(*std::get<MethodIndex>(registration::methods).generic_functor_ptr_, lua);
    }

    template <std::size_t MethodIndex>
    static int method_index_handler(lua_State* lua)
    {
        // stack: 1. this 2. key
        lua_pushcfunction(lua, method_call<MethodIndex>);
        return 1;
    }

    template <std::size_t FieldIndex>
    static int field_index_handler(lua_State* lua)
    {
        // stack: 1. this 2. key
        return [&]<typename V>(bound_field<V T::*>& field) {
            return from_lua<T&>(lua, 1).and_then([&](T& self) -> result<int> {
                return push_to_lua(lua, self.*field.field_ptr_).transform([&]() { return 1; }); // 1 item pushed to stack
            });
        }(std::get<FieldIndex>(registration::fields))
                   .or_else([&](const std::string& error) -> result<int> {
                       lua_pushstring(lua, error.data());
                       lua_error(lua);

                       return unexpected("lua: unreachable post lua_error code reached");
                   })
                   .value();
    }

    template <std::size_t FieldIndex>
    static int field_newindex_handler(lua_State* lua)
    {
        // stack: 1. this 2. key 3. value
        return [&]<typename V>(bound_field<V T::*>& field) {
            return from_lua<T&>(lua, 1).and_then([&](T& self) -> result<int> {
                return from_lua<V>(lua, 3).and_then([&](V value) -> result<int> {
                    if constexpr (std::is_const_v<V>) {
                        return unexpected("lua: attempt to set const field");
                    } else {
                        (self.*field.field_ptr_) = std::move(value);
                    }

                    return 0;
                });
            });
        }(std::get<FieldIndex>(registration::fields))
                   .or_else([&](const std::string& error) -> result<int> {
                       lua_pushstring(lua, error.data());
                       lua_error(lua);

                       return unexpected("lua: unreachable post lua_error code reached");
                   })
                   .value();
    }

    template <std::size_t I, typename M>
    static void add_method(lua_State* lua, bound_method<M>& method)
    {
        lua_pushstring(lua, method.name_.data());
        lua_pushcfunction(lua, method_call<I>);
        lua_settable(lua, -3);
    }

    static int metatable__index_handler(lua_State* lua)
    {
        // stack: 1. this 2. key
        return from_lua<std::string>(lua, 2).and_then([&](std::string value) -> result<int> {
                                                auto pos = index_handlers.find(value);
                                                if (pos != index_handlers.end()) {
                                                    return pos->second(lua);
                                                } else {
                                                    return unexpected(std::format("no field or method '{}'", value));
                                                }
                                            })
            .or_else([&](const std::string& error) -> result<int> {
                lua_pushstring(lua, error.data());
                lua_error(lua);

                return unexpected("lua: unreachable post lua_error code reached");
            })
            .value();
    }

    static int metatable__newindex_handler(lua_State* lua)
    {
        // stack: 1. this 2. key 3. value
        return from_lua<std::string>(lua, 2).and_then([&](std::string value) -> result<int> {
                                                auto pos = newindex_handlers.find(value);
                                                if (pos != newindex_handlers.end()) {
                                                    return pos->second(lua);
                                                } else {
                                                    return unexpected(std::format("no field or method '{}'", value));
                                                }
                                            })
            .or_else([&](const std::string& error) -> result<int> {
                lua_pushstring(lua, error.data());
                lua_error(lua);

                return unexpected("lua: unreachable post lua_error code reached");
            })
            .value();
    }

    static void do_registration(lua_State* lua)
    {
        luaL_newmetatable(lua, registration::name.data());

        lua_pushstring(lua, "__gc");
        lua_pushcfunction(lua, &destructor);
        lua_settable(lua, -3);

        lua_pushstring(lua, "__index");
        lua_pushcfunction(lua, metatable__index_handler);
        lua_settable(lua, -3);

        lua_pushstring(lua, "__newindex");
        lua_pushcfunction(lua, metatable__newindex_handler);
        lua_settable(lua, -3);

        // pop the new metatable off the stack
        lua_pop(lua, 1);

        // add global same-name function for constructor
        lua_pushstring(lua, registration::name.data()); // env__index, "name"
        lua_pushcfunction(lua, constructor); // env__index, "name", constructor
        lua_settable(lua, -3); // env__index
    }

    static void do_deregistration() { }

    using methods_tuple = decltype(registration::methods);
    static constexpr std::size_t num_methods = std::tuple_size_v<methods_tuple>;

    using fields_tuple = decltype(registration::fields);
    static constexpr std::size_t num_fields = std::tuple_size_v<fields_tuple>;

    using index_handler = int (*)(lua_State*);
    static inline std::map<std::string, index_handler> index_handlers = []() {
        std::map<std::string, index_handler> result;
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((result[std::get<Is>(registration::methods).name_] = method_index_handler<Is>), ...);
        }(std::make_index_sequence<num_methods> {});
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((result[std::get<Is>(registration::fields).name_] = field_index_handler<Is>), ...);
        }(std::make_index_sequence<num_fields> {});
        return result;
    }();
    static inline std::map<std::string, index_handler> newindex_handlers = []() {
        std::map<std::string, index_handler> result;
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((result[std::get<Is>(registration::fields).name_] = field_newindex_handler<Is>), ...);
        }(std::make_index_sequence<num_fields> {});
        return result;
    }();
};

template <registered_class T>
struct converter<T*> {
    using impl = class_registration_impl<T>;

    static result<void> push_to_lua(lua_State* lua, T* v)
    {
        return impl::push_wrapped_object(lua, v);
    }

    static result<T*> from_lua(lua_State* lua, lua_Integer i) { return impl::unwrap_object(lua, i); }
};

template <registered_class T>
struct converter<const T*> {
    using impl = class_registration_impl<T>;

    static result<void> push_to_lua(lua_State* lua, const T* v)
    {
        return impl::push_wrapped_object(lua, v);
    }

    static result<const T*> from_lua(lua_State* lua, lua_Integer i) { return impl::unwrap_object_const(lua, i); }
};

template <registered_class T>
struct converter<std::unique_ptr<T>> {
    using impl = class_registration_impl<T>;

    static result<void> push_to_lua(lua_State* lua, std::unique_ptr<T> v)
    {
        return impl::push_wrapped_object(lua, std::move(v));
    }
};

template <registered_class T>
struct converter<std::unique_ptr<const T>> {
    using impl = class_registration_impl<T>;

    static result<void> push_to_lua(lua_State* lua, std::unique_ptr<const T> v)
    {
        return impl::push_wrapped_object(lua, std::move(v));
    }
};

template <registered_class T>
struct converter<T&> {
    using impl = class_registration_impl<T>;

    static result<void> push_to_lua(lua_State* lua, T& v)
    {
        return impl::push_wrapped_object(lua, &v);
    }

    static result<std::reference_wrapper<T>> from_lua(lua_State* lua, lua_Integer i)
    {
        return impl::unwrap_object(lua, i).transform([](auto* ptr) { return std::ref(*ptr); });
    }
};

template <registered_class T>
struct converter<const T&> {
    using impl = class_registration_impl<T>;

    static result<void> push_to_lua(lua_State* lua, const T& v)
    {
        return impl::push_wrapped_object(lua, &v);
    }

    static result<std::reference_wrapper<const T>> from_lua(lua_State* lua, lua_Integer i)
    {
        return impl::unwrap_object_const(lua, i).transform([](const T* ptr) { return std::cref(*ptr); });
    }
};

template <registered_class T>
struct converter<std::reference_wrapper<T>> {
    using impl = class_registration_impl<T>;

    static result<void> push_to_lua(lua_State* lua, std::reference_wrapper<T> v)
    {
        return impl::push_wrapped_object(lua, &v.get());
    }

    static result<std::reference_wrapper<T>> from_lua(lua_State* lua, lua_Integer i)
    {
        return impl::unwrap_object(lua, i).transform([](auto* ptr) { return std::ref(*ptr); });
    }
};

template <registered_class T>
struct converter<std::reference_wrapper<const T>> {
    using impl = class_registration_impl<T>;

    static result<void> push_to_lua(lua_State* lua, std::reference_wrapper<const T> v)
    {
        return impl::push_wrapped_object(lua, &v.get());
    }

    static result<std::reference_wrapper<const T>> from_lua(lua_State* lua, lua_Integer i)
    {
        return impl::unwrap_object_const(lua, i).transform([](const T* ptr) { return std::cref(*ptr); });
    }
};

template <typename ReturnType, typename... ArgTypes>
int call_generic_wrapped_functor(generic_functor<ReturnType, ArgTypes...>& f, lua_State* lua)
{
    int num_args = lua_gettop(lua);
    if (num_args != sizeof...(ArgTypes)) {
        lua_pushstring(lua, "incorrect number of arguments");
        lua_error(lua); // throws, no return
    }

    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        // first arg is at index 1
        return many_from_lua<ArgTypes...>(lua, (1 + Is)...).and_then([&](auto args) -> result<int> {
            if constexpr (std::same_as<ReturnType, void>) {
                std::apply(
                    [&](auto&&... unwrapped_args) { f.call(std::forward<decltype(unwrapped_args)>(unwrapped_args)...); }, std::move(args));

                return 0;
            } else {
                return push_to_lua(lua, std::apply([&](auto&&... unwrapped_args) -> ReturnType { return f.call(std::forward<decltype(unwrapped_args)>(unwrapped_args)...); }, std::move(args)))
                    .transform([]() { return 1; }); // 1 for 1 return value
            }
        });
    }(std::index_sequence_for<ArgTypes...> {})
               .or_else([&](const auto& error) -> result<int> {
                   lua_pushstring(lua, error.data());
                   lua_error(lua);

                   return unexpected("lua: unreachable post lua_error code reached");
               })
               .value();
}

template <typename ReturnType, typename... ArgTypes>
int generic_wrapped_functor(lua_State* lua)
{
    // functor pointer stored as upvalue
    auto* f = static_cast<generic_functor<ReturnType, ArgTypes...>*>(lua_touserdata(lua, lua_upvalueindex(1)));
    return call_generic_wrapped_functor(*f, lua);
}

template <typename ReturnType, typename... ArgTypes>
auto callback_for(const generic_functor<ReturnType, ArgTypes...>&)
{
    return &generic_wrapped_functor<ReturnType, ArgTypes...>;
}

class backend {
public:
    static result<std::unique_ptr<backend>> create(bool start_sandboxed = true)
    {
        return std::unique_ptr<backend>(new backend { start_sandboxed });
    }

    template <typename ReturnType>
    result<ReturnType> execute_script(const std::string& code)
    {
        auto top = lua_gettop(lua_);
        auto retval = [&]() -> result<ReturnType> {
            auto result_code = luaL_loadbuffer(lua_, code.data(),
                code.size(), "glua-lua");

            if (result_code == 0) {
                push_env();
                lua_setfenv(lua_, -2);
                result_code = lua_pcall(lua_, 0, std::same_as<ReturnType, void> ? 0 : 1, 0);

                if (result_code != 0) {
                    return unexpected(std::format("Failed to call script: {}", lua_tostring(lua_, -1)));
                }

                if constexpr (std::same_as<ReturnType, void>) {
                    return {};
                } else {
                    return from_lua<ReturnType>(lua_, -1);
                }
            } else {
                return unexpected(std::format("Failed to load script: {}", lua_tostring(lua_, -1)));
            }
        }();
        lua_settop(lua_, top);

        return retval;
    }

    template <typename ReturnType, typename... ArgTypes>
    result<void> register_functor(const std::string& name, generic_functor<ReturnType, ArgTypes...>& functor)
    {
        push_env__index();
        lua_pushstring(lua_, name.data());

        lua_pushlightuserdata(lua_, &functor);
        lua_pushcclosure(lua_, callback_for(functor), 1);

        lua_settable(lua_, -3); // stack was: env__index, name, closure

        lua_pop(lua_, 1); // pop env__index back off the stack

        return {};
    }

    template <typename T>
    result<T> get_global(const std::string& name)
    {
        push_env();
        lua_pushstring(lua_, name.data());

        lua_gettable(lua_, -2);

        auto result = from_lua<T>(lua_, -2);
        lua_pop(lua_, 2); // pop value and env off stack

        return result;
    }

    template <typename T>
    result<void> set_global(const std::string& name, T value)
    {
        push_env();
        lua_pushstring(lua_, name.data());

        return push_to_lua(lua_, std::move(value))
            .transform([&]() {
                lua_settable(lua_, -3); // stack was: env, name, value
                lua_pop(lua_, 1); // only env__index left on stack
            })
            .transform_error([&](auto error) {
                lua_pop(lua_, 2); // pop name and env
                return error;
            });

        return {};
    }

    template <typename ReturnType, typename... Args>
    result<ReturnType> call_function(const std::string& name, Args&&... args)
    {
        auto starting_top = lua_gettop(lua_);

        // push function then args in order
        push_env(); // env
        lua_pushstring(lua_, name.data()); // env, "name"

        lua_gettable(lua_, -2); // env, env["name"]
        lua_pushvalue(lua_, -2); // env, env["name"], env
        lua_setfenv(lua_, -2); // env, env["name"]

        auto retval = many_push_to_lua(lua_, std::forward<Args>(args)...).and_then([&]() -> result<ReturnType> {
            // stack now: env, env["name"], args...
            auto call_result = lua_pcall(lua_, sizeof...(Args), std::same_as<ReturnType, void> ? 0 : 1, 0);

            if (call_result != 0) {
                return unexpected(std::format("lua call failed: {}", lua_tostring(lua_, -1)));
            }

            if constexpr (std::same_as<ReturnType, void>) {
                return {};
            } else {
                return from_lua<ReturnType>(lua_, -1);
            }
        });

        // pop off anything leftover, as many_push_to_lua could have failed mid-pushing
        // so we can't determine the number of items to pop at compile time
        lua_settop(lua_, starting_top);

        return retval;
    }

    template <registered_class T>
    void register_class()
    {
        push_env__index(); // push env__index so registration can add globals if needed
        class_registration_impl<T>::do_registration(lua_);
        lua_pop(lua_, 1); // remove env__index
    }

    void push_env()
    {
        lua_getglobal(lua_, env_name);
    }

    void push_env__index()
    {
        lua_getglobal(lua_, env__index_name);
    }

    ~backend()
    {
        lua_close(lua_);
    }

private:
    backend(bool start_sandboxed)
        : lua_(luaL_newstate())
    {
        luaL_openlibs(lua_);

        build_sandbox(start_sandboxed);

        // let's create the env table
        lua_newtable(lua_); // env
        lua_pushvalue(lua_, -1); // env, env
        lua_pushliteral(lua_, "__index"); // env, env, __index
        push_env__index(); // env, env, __index, env__index
        lua_settable(lua_, -3); // env, env
        lua_setmetatable(lua_, -2); // env

        lua_setglobal(lua_, env_name); // empty stack
    }

    void build_sandbox(bool start_sandboxed)
    {
        if (start_sandboxed) {
            // lookup the global for each of these and push them onto env
            std::vector<std::string> sandbox_environment {
                "assert", "error", "ipairs", "next", "pairs",
                "pcall", "print", "select", "tonumber", "tostring",
                "type", "unpack", "_VERSION", "xpcall", "isfunction"
            };
            std::map<std::string, std::vector<std::string>> sandbox_sub_environments = {
                { "coroutine", std::vector<std::string> { "create", "resume", "running", "status", "wrap", "yield" } },
                { "io", std::vector<std::string> { "read", "write", "flush", "type" } },
                { "string", std::vector<std::string> { "byte", "char", "dump", "find", "format", "gmatch", "gsub", "len", "lower", "match", "rep", "reverse", "sub", "upper" } },
                { "table", std::vector<std::string> { "insert", "maxn", "remove", "sort", "concat" } },
                { "math", std::vector<std::string> { "abs", "acos", "asin", "atan", "atan2", "ceil", "cos", "cosh", "deg", "exp", "floor", "fmod", "frexp", "huge", "ldexp", "log", "log10", "max", "min", "modf", "pi", "pow", "rad", "random", "sin", "sinh", "sqrt", "tan", "tanh" } },
                { "os", std::vector<std::string> { "clock", "difftime", "time" } }
            };

            lua_newtable(lua_); // env__index;
            for (const auto& f : sandbox_environment) {
                lua_pushstring(lua_, f.data());
                lua_getglobal(lua_, f.data());
                lua_settable(lua_, -3); // set env__index["name"] = globals["name"]
            }

            // top of stack is still env__index
            for (const auto& [sub_environment_name, items] : sandbox_sub_environments) {
                lua_pushstring(lua_, sub_environment_name.data()); // env__index, "subenv"
                lua_newtable(lua_); // env__index, "subenv", subenv
                lua_getglobal(lua_, sub_environment_name.data()); // env__index, "subenv", subenv, globals["subenv"]
                for (const auto& f : items) {
                    // need to get f from coroutine table, push onto env_index["subenv"];
                    lua_pushstring(lua_, f.data()); // env__index, "subenv", subenv, globals["subenv"], "name"
                    lua_pushvalue(lua_, -1); // env__index, "subenv", subenv, globals["subenv"], "name", "name"
                    lua_gettable(lua_, -3); // env__index, "subenv", subenv, globals["subenv"], "name", globals["subenv"]["name"]
                    lua_settable(lua_, -4); // env__index, "subenv", subenv, globals["subenv"]
                }
                lua_pop(lua_, 1); // env__index, "subenv", subenv
                lua_settable(lua_, -3); // env__index
            }

            lua_setglobal(lua_, env__index_name); // stack now back to start
        } else {
            // when not using a sandbox:
            //   env.__index -> _G
            // where _G is the default lua globals table
            lua_getglobal(lua_, "_G");
            lua_setglobal(lua_, env__index_name);
        }
    }

    static constexpr char env_name[] = "__glua_env";
    static constexpr char env__index_name[] = "__glua_env__index";

    lua_State* lua_;
};

} // namespace glua::lua
