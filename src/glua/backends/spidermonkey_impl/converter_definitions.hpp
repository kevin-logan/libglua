#pragma once

// NOTE: Do not include this, include glua/backends/spidermonkey.hpp instead, the include order is carefully
// crafted to separate declarations and dependent definitions

namespace glua::spidermonkey {
template <decays_to<std::string> T>
result<std::string> converter<T>::from_js(JSContext* cx, JS::HandleValue v)
{
    JS::RootedString str { cx, JS::ToString(cx, v) };
    auto size = JS_GetStringEncodingLength(cx, str);
    std::string retval;
    retval.resize(size);
    if (!JS_EncodeStringToBuffer(cx, str, retval.data(), size)) {
        return unexpected("Could not encode js string to buffer");
    }

    // might contain '\0' that we still need to trim...?

    return retval;
}

template <decays_to<std::string_view> T>
result<JS::Value> converter<T>::to_js(JSContext* cx, std::string_view v)
{
    return JS::StringValue(JS_NewStringCopyN(cx, v.data(), v.size()));
}

template <decays_to_vector T>
result<JS::Value> converter<T>::to_js(JSContext* cx, T v)
{
    return converter<std::decay_t<T>>::to_js(cx, v);
}

template <decays_to_unordered_map T>
result<JS::Value> converter<T>::to_js(JSContext* cx, T v)
{
    return converter<std::decay_t<T>>::to_js(cx, v);
}

inline result<JS::Value> converter<int8_t>::to_js(JSContext*, int8_t v)
{
    JS::Value result;
    result.setNumber(v);
    return result;
}

inline result<int8_t> converter<int8_t>::from_js(JSContext* cx, JS::HandleValue v)
{
    int8_t retval;
    JS::ToInt8(cx, v, &retval);

    return retval;
}

inline result<JS::Value> converter<uint8_t>::to_js(JSContext*, uint8_t v)
{
    JS::Value result;
    result.setNumber(v);
    return result;
}

inline result<uint8_t> converter<uint8_t>::from_js(JSContext* cx, JS::HandleValue v)
{
    uint8_t retval;
    JS::ToUint8(cx, v, &retval);

    return retval;
}

inline result<JS::Value> converter<int16_t>::to_js(JSContext*, int16_t v)
{
    JS::Value result;
    result.setNumber(v);
    return result;
}

inline result<int16_t> converter<int16_t>::from_js(JSContext* cx, JS::HandleValue v)
{
    int16_t retval;
    JS::ToInt16(cx, v, &retval);

    return retval;
}

inline result<JS::Value> converter<uint16_t>::to_js(JSContext*, uint16_t v)
{
    JS::Value result;
    result.setNumber(v);
    return result;
}

inline result<uint16_t> converter<uint16_t>::from_js(JSContext* cx, JS::HandleValue v)
{
    uint16_t retval;
    JS::ToUint16(cx, v, &retval);

    return retval;
}

inline result<JS::Value> converter<int32_t>::to_js(JSContext*, int32_t v) { return JS::Int32Value(v); }

inline result<int32_t> converter<int32_t>::from_js(JSContext* cx, JS::HandleValue v)
{
    int32_t retval;
    JS::ToInt32(cx, v, &retval);

    return retval;
}

inline result<JS::Value> converter<uint32_t>::to_js(JSContext*, uint32_t v)
{
    JS::Value result;
    result.setNumber(v);
    return result;
}

inline result<uint32_t> converter<uint32_t>::from_js(JSContext* cx, JS::HandleValue v)
{
    uint32_t retval;
    JS::ToUint32(cx, v, &retval);

    return retval;
}

inline result<JS::Value> converter<int64_t>::to_js(JSContext*, int64_t v)
{
    JS::Value result;
    result.setNumber(v);
    return result;
}

inline result<int64_t> converter<int64_t>::from_js(JSContext* cx, JS::HandleValue v)
{
    int64_t retval;
    JS::ToInt64(cx, v, &retval);

    return retval;
}

inline result<JS::Value> converter<uint64_t>::to_js(JSContext*, uint64_t v)
{
    JS::Value result;
    result.setNumber(v);
    return result;
}

inline result<uint64_t> converter<uint64_t>::from_js(JSContext* cx, JS::HandleValue v)
{
    uint64_t retval;
    JS::ToUint64(cx, v, &retval);

    return retval;
}

inline result<JS::Value> converter<float>::to_js(JSContext*, float v) { return JS::Float32Value(v); }

inline result<float> converter<float>::from_js(JSContext* cx, JS::HandleValue v)
{
    double retval;
    JS::ToNumber(cx, v, &retval);

    return static_cast<float>(retval);
}

inline result<JS::Value> converter<double>::to_js(JSContext*, double v) { return JS::DoubleValue(v); }

inline result<double> converter<double>::from_js(JSContext* cx, JS::HandleValue v)
{
    double retval;
    JS::ToNumber(cx, v, &retval);

    return retval;
}

inline result<JS::Value> converter<bool>::to_js(JSContext*, bool v) { return JS::BooleanValue(v); }

inline result<bool> converter<bool>::from_js(JSContext*, JS::HandleValue v) { return JS::ToBoolean(v); }

template <registered_class T>
result<JS::Value> converter<T*>::to_js(JSContext* cx, T* v)
{
    return class_registration_impl<T>::wrap_object(cx, v).transform([](auto* obj) { return JS::ObjectValue(*obj); });
}

template <registered_class T>
result<T*> converter<T*>::from_js(JSContext* cx, JS::HandleValue v) { return class_registration_impl<T>::unwrap_object(cx, v); }

template <registered_class T>
result<JS::Value> converter<const T*>::to_js(JSContext* cx, const T* v)
{
    return class_registration_impl<T>::wrap_object(cx, v).transform([](auto* obj) { return JS::ObjectValue(*obj); });
}

template <registered_class T>
result<const T*> converter<const T*>::from_js(JSContext* cx, JS::HandleValue v) { return class_registration_impl<T>::unwrap_object_const(cx, v); }

template <registered_class T>
result<JS::Value> converter<std::unique_ptr<T>>::to_js(JSContext* cx, std::unique_ptr<T> v)
{
    return class_registration_impl<T>::wrap_object(cx, std::move(v)).transform([](auto* obj) { return JS::ObjectValue(*obj); });
}

template <registered_class T>
result<JS::Value> converter<std::unique_ptr<const T>>::to_js(JSContext* cx, std::unique_ptr<const T> v)
{
    return class_registration_impl<T>::wrap_object(cx, std::move(v)).transform([](auto* obj) { return JS::ObjectValue(*obj); });
}

template <registered_class T>
result<JS::Value> converter<T&>::to_js(JSContext* cx, T& v)
{
    return class_registration_impl<T>::wrap_object(cx, &v).transform([](auto* obj) { return JS::ObjectValue(*obj); });
}

template <registered_class T>
result<std::reference_wrapper<T>> converter<T&>::from_js(JSContext* cx, JS::HandleValue v)
{
    return class_registration_impl<T>::unwrap_object(cx, v).transform([](auto* ptr) { return std::ref(*ptr); });
}

template <registered_class T>
result<JS::Value> converter<const T&>::to_js(JSContext* cx, const T& v)
{
    return class_registration_impl<T>::wrap_object(cx, &v).transform([](auto* obj) { return JS::ObjectValue(*obj); });
}

template <registered_class T>
result<std::reference_wrapper<const T>> converter<const T&>::from_js(JSContext* cx, JS::HandleValue v)
{
    return class_registration_impl<T>::unwrap_object_const(cx, v).transform([](const T* ptr) { return std::cref(*ptr); });
}

template <registered_class T>
result<JS::Value> converter<std::reference_wrapper<T>>::to_js(JSContext* cx, std::reference_wrapper<T> v)
{
    return class_registration_impl<T>::wrap_object(cx, &v.get()).transform([](auto* obj) { return JS::ObjectValue(*obj); });
}

template <registered_class T>
result<std::reference_wrapper<T>> converter<std::reference_wrapper<T>>::from_js(JSContext* cx, JS::HandleValue v)
{
    return class_registration_impl<T>::unwrap_object(cx, v).transform([](auto* ptr) { return std::ref(*ptr); });
}

template <registered_class T>
result<JS::Value> converter<std::reference_wrapper<const T>>::to_js(JSContext* cx, std::reference_wrapper<const T> v)
{
    return class_registration_impl<T>::wrap_object(cx, &v.get()).transform([](auto* obj) { return JS::ObjectValue(*obj); });
}

template <registered_class T>
result<std::reference_wrapper<const T>> converter<std::reference_wrapper<const T>>::from_js(JSContext* cx, JS::HandleValue v)
{
    return class_registration_impl<T>::unwrap_object_const(cx, v).transform([](const T* ptr) { return std::cref(*ptr); });
}

template <typename T>
result<JS::Value> converter<std::vector<T>>::to_js(JSContext* cx, std::vector<T>&& v)
{
    const std::size_t s = v.size();
    JS::RootedObject obj { cx, JS::NewArrayObject(cx, s) };
    for (std::size_t i = 0; i < s; ++i) {
        auto sub_result = spidermonkey::to_js(cx, std::move(v[i]));
        if (sub_result.has_value()) {
            JS::RootedValue sub_value { cx, sub_result.value() };
            JS_SetElement(cx, obj, i, sub_value);
        } else {
            return unexpected(std::move(sub_result).error());
        }
    }

    return JS::ObjectOrNullValue(obj);
}

template <typename T>
result<JS::Value> converter<std::vector<T>>::to_js(JSContext* cx, const std::vector<T>& v)
{
    const std::size_t s = v.size();
    JS::RootedObject obj { cx, JS::NewArrayObject(cx, s) };
    for (std::size_t i = 0; i < s; ++i) {
        auto sub_result = spidermonkey::to_js(cx, v[i]);
        if (sub_result.has_value()) {
            JS::RootedValue sub_value { cx, sub_result.value() };
            JS_SetElement(cx, obj, i, sub_value);
        } else {
            return unexpected(std::move(sub_result).error());
        }
    }

    return JS::ObjectOrNullValue(obj);
}

template <typename T>
result<std::vector<T>> converter<std::vector<T>>::from_js(JSContext* cx, JS::HandleValue v)
{
    if (v.isObject()) {
        bool is_array { false };
        if (JS::IsArrayObject(cx, v, &is_array) && is_array) {
            JS::RootedObject array_obj { cx, &v.toObject() };
            uint32_t length;
            if (JS::GetArrayLength(cx, array_obj, &length)) {
                std::vector<T> result;
                result.reserve(length);

                for (uint32_t i = 0; i < length; ++i) {
                    JS::RootedValue val { cx };
                    if (JS_GetElement(cx, array_obj, i, &val)) {
                        auto inner_result = spidermonkey::from_js<T>(cx, val);
                        if (inner_result.has_value()) {
                            result.push_back(std::move(inner_result).value());
                        } else {
                            return unexpected(std::move(inner_result).error());
                        }
                    } else {
                        return unexpected("Could not get array element");
                    }
                }

                return result;
            } else {
                return unexpected("Could not get size of array");
            }
        } else {
            return unexpected("Could not build vector from non-array object");
        }
    } else {
        return unexpected("Could not convert non-object to array");
    }
}

template <typename T>
result<JS::Value> converter<std::unordered_map<std::string, T>>::to_js(JSContext* cx, std::unordered_map<std::string, T>&& v)
{
    JS::RootedObject js_obj { cx, JS_NewPlainObject(cx) };

    for (auto& [key, value] : v) {
        auto result = spidermonkey::to_js(cx, std::move(value)).transform([&](auto value_value) {
            JS::RootedValue rooted_value { cx, value_value };
            JS_SetProperty(cx, js_obj, key.data(), rooted_value);
        });
        if (!result.has_value())
            return unexpected(std::move(result).error());
    }

    return JS::ObjectOrNullValue(js_obj);
}

template <typename T>
result<JS::Value> converter<std::unordered_map<std::string, T>>::to_js(JSContext* cx, const std::unordered_map<std::string, T>& v)
{
    JS::RootedObject js_obj { cx, JS_NewPlainObject(cx) };

    for (const auto& [key, value] : v) {
        auto result = spidermonkey::to_js(cx, value).transform([&](auto value_value) {
            JS::RootedValue rooted_value { cx, value_value };
            JS_SetProperty(cx, js_obj, key.data(), rooted_value);
        });
        if (!result.has_value())
            return unexpected(std::move(result).error());
    }

    return JS::ObjectOrNullValue(js_obj);
}

template <typename T>
result<std::unordered_map<std::string, T>> converter<std::unordered_map<std::string, T>>::from_js(JSContext* cx, JS::HandleValue v)
{
    if (v.isObject()) {
        JS::RootedObject map_obj { cx, &v.toObject() };
        std::unordered_map<std::string, T> map;

        JS::Rooted<JS::IdVector> ids { cx, JS::IdVector(cx) };

        if (JS_Enumerate(cx, map_obj, &ids)) {
            const std::size_t length = ids.length();
            map.reserve(length);
            for (std::size_t i = 0; i < length; ++i) {
                JS::RootedId element_id { cx, ids[i] };
                JS::RootedValue element_value { cx };

                if (JS_GetPropertyById(cx, map_obj, element_id, &element_value)) {
                    JS::RootedString key_str { cx, element_id.toString() };
                    auto key_str_size = JS_GetStringEncodingLength(cx, key_str);
                    std::string key_str_cpp;
                    key_str_cpp.resize(key_str_size);
                    if (!JS_EncodeStringToBuffer(cx, key_str, key_str_cpp.data(), key_str_size)) {
                        return unexpected("Could not encode property key to buffer");
                    }

                    auto value_result = spidermonkey::from_js<T>(cx, element_value);
                    if (value_result.has_value()) {
                        map.emplace(std::move(key_str_cpp), std::move(value_result).value());
                    } else {
                        return unexpected("Could not convert property value to the requested typed");
                    }
                } else {
                    return unexpected("Could not retrieve property value");
                }
            }

            return map;
        } else {
            return unexpected("Could not enumerate object in map conversion");
        }
    } else {
        return unexpected("Could not convert non-object to map");
    }
}

inline result<JS::Value> converter<any>::to_js(JSContext* cx, any&& v)
{
    auto* spidermonkey_any = dynamic_cast<any_spidermonkey_impl*>(v.impl_.get());
    if (spidermonkey_any) {
        return std::move(*spidermonkey_any).to_js(cx);
    } else {
        return unexpected("Received invalid any, perhaps from another backend");
    }
}

inline result<any> converter<any>::from_js(JSContext* cx, JS::HandleValue v)
{
    // have to figure out what v really is and build the correct representation
    return [&]() -> result<std::unique_ptr<any_impl>> {
        switch (v.type()) {
        case JS::ValueType::Double:
            return spidermonkey::from_js<double>(cx, v).transform([&](auto value) -> std::unique_ptr<any_impl> {
                return std::make_unique<any_basic_impl>(value);
            });
        case JS::ValueType::Int32:
            return spidermonkey::from_js<int32_t>(cx, v).transform([&](auto value) -> std::unique_ptr<any_impl> {
                return std::make_unique<any_basic_impl>(value);
            });
        case JS::ValueType::Boolean:
            return spidermonkey::from_js<bool>(cx, v).transform([&](auto value) -> std::unique_ptr<any_impl> {
                return std::make_unique<any_basic_impl>(value);
            });
        case JS::ValueType::String:
            return spidermonkey::from_js<std::string>(cx, v).transform([&](auto value) -> std::unique_ptr<any_impl> {
                return std::make_unique<any_basic_impl>(std::move(value));
            });
        case JS::ValueType::Object: {
            // if registered_class wrap (any_owned_registered_class_impl or any_borrowed_registered_class_impl)
            // if array make vector<any> (any_array_impl)
            // if map make unordered_map<std::string, any> (any_map_impl)
            auto& obj = v.toObject();
            auto* c = JS::GetClass(&obj);
            bool is_array { false };

            if (JSCLASS_RESERVED_SLOTS(c) == SLOT_COUNT) {
                // if it has our SLOT_COUNT it's a registered_class
                JS::RootedObject obj_handle { cx, &obj };
                JS::RootedObject proto_handle { cx };
                if (JS_GetPrototype(cx, obj_handle, JS::MutableHandleObject(&proto_handle))) {
                    return [&]() -> std::unique_ptr<any_impl> { return std::make_unique<any_registered_class_impl>(obj_handle, proto_handle); }();
                } else {
                    return unexpected("Could not retrieve any object prototype");
                }
            } else if (JS::IsArrayObject(cx, v, &is_array) && is_array) {
                return spidermonkey::from_js<std::vector<any>>(cx, v).transform([&](auto value) -> std::unique_ptr<any_impl> {
                    return std::make_unique<any_array_impl<any>>(std::move(value));
                });
            } else {
                return spidermonkey::from_js<std::unordered_map<std::string, any>>(cx, v).transform([&](auto value) -> std::unique_ptr<any_impl> {
                    return std::make_unique<any_map_impl<any>>(std::move(value));
                });
            }
        }
        case JS::ValueType::Undefined:
        case JS::ValueType::Null:
        case JS::ValueType::Magic:
        case JS::ValueType::Symbol:
        case JS::ValueType::PrivateGCThing:
        case JS::ValueType::BigInt:
            return unexpected("Attempted to wrap unsupported value type");
        }

        return unexpected("Could not create any from unrecognized type");
    }()
                        .transform([&](auto any_impl_ptr) {
                            return any { std::move(any_impl_ptr) };
                        });
}

template <typename... Ts>
auto many_to_js([[maybe_unused]] JSContext* cx, Ts&&... values)
{
    auto many_expected = std::make_tuple(to_js(cx, static_cast<Ts&&>(values))...);
    return many_results_to_one(std::move(many_expected));
}

template <typename... Ts>
auto many_from_js([[maybe_unused]] JSContext* cx, HandleValueFor<Ts>... vs)
{
    auto many_expected = std::make_tuple(from_js<Ts>(cx, vs)...);
    return many_results_to_one(std::move(many_expected));
}
}
