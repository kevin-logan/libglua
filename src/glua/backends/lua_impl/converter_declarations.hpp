#pragma once

// NOTE: Do not include this, include glua/backends/lua.hpp instead, the include order is carefully
// crafted to separate declarations and dependent definitions

namespace glua::lua {
int make_absolute(lua_State* lua, int i)
{
    if (i < 0) {
        // -1 would be top, so subtract one fewer
        return lua_gettop(lua) + i + 1;
    } else {
        return i;
    }
}

template <typename T>
auto push_to_lua(lua_State* lua, T&& value);

template <typename ReturnType, typename... ArgTypes>
int call_generic_wrapped_functor(generic_functor<ReturnType, ArgTypes...>& f, lua_State* lua);

template <typename T>
struct converter {
};

template <decays_to<std::string> T>
struct converter<T> {
    static result<void> push_to_lua(lua_State* lua, const std::string& v);

    static result<std::string> from_lua(lua_State* lua, int i);
};

template <decays_to<std::string_view> T>
struct converter<T> {
    static result<void> push_to_lua(lua_State* lua, std::string_view v);

    // from_lua not supported, can't 'point' to script bytes
};

template <typename T>
struct converter<std::vector<T>> {
    static result<void> push_to_lua(lua_State* lua, std::vector<T>&& v);
    static result<void> push_to_lua(lua_State* lua, const std::vector<T>& v);

    static result<std::vector<T>> from_lua(lua_State* lua, int i);
};

template <decays_to_vector T>
struct converter<T> {
    static result<void> push_to_lua(lua_State* lua, T v);

    // don't support from_lua as it's a reference to a vector
};

template <typename T>
struct converter<std::unordered_map<std::string, T>> {
    static result<void> push_to_lua(lua_State* lua, std::unordered_map<std::string, T>&& v);
    static result<void> push_to_lua(lua_State* lua, const std::unordered_map<std::string, T>& v);

    static result<std::unordered_map<std::string, T>> from_lua(lua_State* lua, int i);
};

template <decays_to_unordered_map T>
struct converter<T> {
    static result<void> push_to_lua(lua_State* lua, T v);

    // don't support from_lua as it's a reference to a map
};

template <>
struct converter<int8_t> {
    static result<void> push_to_lua(lua_State* lua, int8_t v);

    static result<int8_t> from_lua(lua_State* lua, int i);
};

template <>
struct converter<uint8_t> {
    static result<void> push_to_lua(lua_State* lua, uint8_t v);

    static result<uint8_t> from_lua(lua_State* lua, int i);
};

template <>
struct converter<int16_t> {
    static result<void> push_to_lua(lua_State* lua, int16_t v);

    static result<int16_t> from_lua(lua_State* lua, int i);
};

template <>
struct converter<uint16_t> {
    static result<void> push_to_lua(lua_State* lua, uint16_t v);

    static result<uint16_t> from_lua(lua_State* lua, int i);
};

template <>
struct converter<int32_t> {
    static result<void> push_to_lua(lua_State* lua, int32_t v);

    static result<int32_t> from_lua(lua_State* lua, int i);
};

template <>
struct converter<uint32_t> {
    static result<void> push_to_lua(lua_State* lua, uint32_t v);

    static result<uint32_t> from_lua(lua_State* lua, int i);
};

template <>
struct converter<int64_t> {
    static result<void> push_to_lua(lua_State* lua, int64_t v);

    static result<int64_t> from_lua(lua_State* lua, int i);
};

template <>
struct converter<uint64_t> {
    static result<void> push_to_lua(lua_State* lua, uint64_t v);

    static result<uint64_t> from_lua(lua_State* lua, int i);
};

template <>
struct converter<float> {
    static result<void> push_to_lua(lua_State* lua, float v);

    static result<float> from_lua(lua_State* lua, int i);
};

template <>
struct converter<double> {
    static result<void> push_to_lua(lua_State* lua, double v);

    static result<double> from_lua(lua_State* lua, int i);
};

template <>
struct converter<bool> {
    static result<void> push_to_lua(lua_State* lua, bool v);

    static result<bool> from_lua(lua_State* lua, int i);
};

template <decays_to_integral T>
struct converter<T> : converter<std::decay_t<T>> { };

template <decays_to<float> T>
struct converter<T> : converter<std::decay_t<T>> { };

template <decays_to<double> T>
struct converter<T> : converter<std::decay_t<T>> { };

template <registered_class T>
struct converter<T*> {
    static result<void> push_to_lua(lua_State* lua, T* v);

    static result<T*> from_lua(lua_State* lua, int i);
};

template <registered_class T>
struct converter<const T*> {
    static result<void> push_to_lua(lua_State* lua, const T* v);

    static result<const T*> from_lua(lua_State* lua, int i);
};

template <registered_class T>
struct converter<std::unique_ptr<T>> {
    static result<void> push_to_lua(lua_State* lua, std::unique_ptr<T> v);
};

template <registered_class T>
struct converter<std::unique_ptr<const T>> {
    static result<void> push_to_lua(lua_State* lua, std::unique_ptr<const T> v);
};

template <registered_class T>
struct converter<T&> {
    static result<void> push_to_lua(lua_State* lua, T& v);

    static result<std::reference_wrapper<T>> from_lua(lua_State* lua, int i);
};

template <registered_class T>
struct converter<const T&> {
    static result<void> push_to_lua(lua_State* lua, const T& v);

    static result<std::reference_wrapper<const T>> from_lua(lua_State* lua, int i);
};

template <registered_class T>
struct converter<std::reference_wrapper<T>> {
    static result<void> push_to_lua(lua_State* lua, std::reference_wrapper<T> v);

    static result<std::reference_wrapper<T>> from_lua(lua_State* lua, int i);
};

template <registered_class T>
struct converter<std::reference_wrapper<const T>> {
    static result<void> push_to_lua(lua_State* lua, std::reference_wrapper<const T> v);

    static result<std::reference_wrapper<const T>> from_lua(lua_State* lua, int i);
};

template <>
struct converter<any> {
    static result<void> push_to_lua(lua_State* lua, any&& v);

    static result<any> from_lua(lua_State* lua, int i);
};

template <typename T, typename... Ts>
result<void> many_push_to_lua(lua_State* lua, T&& value, Ts&&... values);

result<void> many_push_to_lua(lua_State* lua);

template <typename T>
using IndexFor = lua_Integer;

template <typename... Ts>
auto many_from_lua([[maybe_unused]] lua_State* lua, IndexFor<Ts>... vs);

////////////////////////////////////////////////////////////////////
// THESE HELPERS MUST ALWAYS BE THE LAST DEFINITIONS IN THIS FILE //
////////////////////////////////////////////////////////////////////
template <typename T>
auto push_to_lua(lua_State* lua, T&& value)
{
    return converter<T>::push_to_lua(lua, static_cast<T&&>(value));
}

template <typename T>
auto from_lua(lua_State* lua, int i)
{
    return converter<T>::from_lua(lua, i);
}
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
}
