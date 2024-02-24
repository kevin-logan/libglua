#pragma once

// NOTE: Do not include this, include glua/backends/spidermonkey.hpp instead, the include order is carefully
// crafted to separate declarations and dependent definitions

namespace glua::spidermonkey {
template <typename T>
result<JS::Value> to_js(JSContext* cx, T&& value);

template <typename ReturnType, typename... ArgTypes>
bool call_generic_wrapped_functor(generic_functor<ReturnType, ArgTypes...>& f, JSContext* cx, JS::CallArgs& args);

template <typename ReturnType, typename ClassType, typename... ArgTypes>
bool call_generic_wrapped_method(generic_functor<ReturnType, ClassType, ArgTypes...>& f, JSContext* cx, JS::CallArgs& args);

template <typename T>
struct converter {
};

template <decays_to<std::string> T>
struct converter<T> {
    static result<JS::Value> to_js(JSContext* cx, const std::string& v);

    static result<std::string> from_js(JSContext* cx, JS::HandleValue v);
};

template <decays_to<std::string_view> T>
struct converter<T> {
    static result<JS::Value> to_js(JSContext* cx, std::string_view v);

    // from_js not supported, can't 'point' to JS bytes
};

template <decays_to<const char*> T>
struct converter<T> {
    static result<JS::Value> to_js(JSContext* cx, const char* v);

    // from_js not supported, can't 'point' to JS bytes
};

template <decays_to<char*> T>
struct converter<T> : converter<const char*> { };

template <typename T>
struct converter<std::vector<T>> {
    static result<JS::Value> to_js(JSContext* cx, std::vector<T>&& v);
    static result<JS::Value> to_js(JSContext* cx, const std::vector<T>& v);

    static result<std::vector<T>> from_js(JSContext* cx, JS::HandleValue v);
};

template <decays_to_vector T>
struct converter<T> {
    static result<JS::Value> to_js(JSContext* cx, T v);

    // don't support from_js as it's a reference to a vector
};

template <typename T>
struct converter<std::unordered_map<std::string, T>> {
    static result<JS::Value> to_js(JSContext* cx, std::unordered_map<std::string, T>&& v);
    static result<JS::Value> to_js(JSContext* cx, const std::unordered_map<std::string, T>& v);

    static result<std::unordered_map<std::string, T>> from_js(JSContext* cx, JS::HandleValue v);
};

template <decays_to_unordered_map T>
struct converter<T> {
    static result<JS::Value> to_js(JSContext* cx, T v);

    // don't support from_js as it's a reference to a map
};

template <>
struct converter<int8_t> {
    static result<JS::Value> to_js(JSContext*, int8_t v);

    static result<int8_t> from_js(JSContext* cx, JS::HandleValue v);
};

template <>
struct converter<uint8_t> {
    static result<JS::Value> to_js(JSContext*, uint8_t v);

    static result<uint8_t> from_js(JSContext* cx, JS::HandleValue v);
};

template <>
struct converter<int16_t> {
    static result<JS::Value> to_js(JSContext*, int16_t v);

    static result<int16_t> from_js(JSContext* cx, JS::HandleValue v);
};

template <>
struct converter<uint16_t> {
    static result<JS::Value> to_js(JSContext*, uint16_t v);

    static result<uint16_t> from_js(JSContext* cx, JS::HandleValue v);
};

template <>
struct converter<int32_t> {
    static result<JS::Value> to_js(JSContext*, int32_t v);

    static result<int32_t> from_js(JSContext* cx, JS::HandleValue v);
};

template <>
struct converter<uint32_t> {
    static result<JS::Value> to_js(JSContext*, uint32_t v);

    static result<uint32_t> from_js(JSContext* cx, JS::HandleValue v);
};

template <>
struct converter<int64_t> {
    static result<JS::Value> to_js(JSContext*, int64_t v);

    static result<int64_t> from_js(JSContext* cx, JS::HandleValue v);
};

template <>
struct converter<uint64_t> {
    static result<JS::Value> to_js(JSContext*, uint64_t v);

    static result<uint64_t> from_js(JSContext* cx, JS::HandleValue v);
};

template <>
struct converter<float> {
    static result<JS::Value> to_js(JSContext*, float v);

    static result<float> from_js(JSContext* cx, JS::HandleValue v);
};

template <>
struct converter<double> {
    static result<JS::Value> to_js(JSContext*, double v);

    static result<double> from_js(JSContext* cx, JS::HandleValue v);
};

template <>
struct converter<bool> {
    static result<JS::Value> to_js(JSContext*, bool v);

    static result<bool> from_js(JSContext*, JS::HandleValue v);
};

template <>
struct converter<any> {
    static result<JS::Value> to_js(JSContext* cx, const any& v);

    static result<any> from_js(JSContext* cx, JS::HandleValue v);
};

template <decays_to<any> T>
struct converter<T> : converter<std::decay_t<T>> { };

template <decays_to_integral T>
struct converter<T> : converter<std::decay_t<T>> { };

template <decays_to<float> T>
struct converter<T> : converter<std::decay_t<T>> { };

template <decays_to<double> T>
struct converter<T> : converter<std::decay_t<T>> { };

template <registered_class T>
struct converter<T*> {
    static result<JS::Value> to_js(JSContext* cx, T* v);

    static result<T*> from_js(JSContext* cx, JS::HandleValue v);
};

template <registered_class T>
struct converter<const T*> {
    static result<JS::Value> to_js(JSContext* cx, const T* v);

    static result<const T*> from_js(JSContext* cx, JS::HandleValue v);
};

template <registered_class T>
struct converter<std::unique_ptr<T>> {
    static result<JS::Value> to_js(JSContext* cx, std::unique_ptr<T> v);
};

template <registered_class T>
struct converter<std::unique_ptr<const T>> {
    static result<JS::Value> to_js(JSContext* cx, std::unique_ptr<const T> v);
};

template <registered_class T>
struct converter<T&> {
    static result<JS::Value> to_js(JSContext* cx, T& v);

    static result<std::reference_wrapper<T>> from_js(JSContext* cx, JS::HandleValue v);
};

template <registered_class T>
struct converter<const T&> {
    static result<JS::Value> to_js(JSContext* cx, const T& v);

    static result<std::reference_wrapper<const T>> from_js(JSContext* cx, JS::HandleValue v);
};

template <registered_class T>
struct converter<std::reference_wrapper<T>> {
    static result<JS::Value> to_js(JSContext* cx, std::reference_wrapper<T> v);

    static result<std::reference_wrapper<T>> from_js(JSContext* cx, JS::HandleValue v);
};

template <registered_class T>
struct converter<std::reference_wrapper<const T>> {
    static result<JS::Value> to_js(JSContext* cx, std::reference_wrapper<const T> v);

    static result<std::reference_wrapper<const T>> from_js(JSContext* cx, JS::HandleValue v);
};

template <typename T>
using ValueFor = JS::Value;

template <typename... Ts>
auto many_to_js([[maybe_unused]] JSContext* cx, Ts&&... values);

template <typename T>
using HandleValueFor = JS::HandleValue;

template <typename... Ts>
auto many_from_js([[maybe_unused]] JSContext* cx, HandleValueFor<Ts>... vs);

template <typename T>
result<JS::Value> to_js(JSContext* cx, T&& value)
{
    return converter<T>::to_js(cx, static_cast<T&&>(value));
}

////////////////////////////////////////////////////////////////////
// THESE HELPERS MUST ALWAYS BE THE LAST DEFINITIONS IN THIS FILE //
////////////////////////////////////////////////////////////////////
template <decays_to<std::string> T>
result<JS::Value> converter<T>::to_js(JSContext* cx, const std::string& v)
{
    return JS::StringValue(JS_NewStringCopyN(cx, v.data(), v.size()));
}

template <typename T>
auto from_js(JSContext* cx, JS::HandleValue v)
{
    return converter<T>::from_js(cx, v);
}
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
}
