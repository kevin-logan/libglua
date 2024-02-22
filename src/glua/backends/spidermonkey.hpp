#pragma once

#include "glua/glua.hpp"

#include <jsapi.h>
#include <jsfriendapi.h>

#include <js/CallArgs.h>
#include <js/CompilationAndEvaluation.h>
#include <js/Conversions.h>
#include <js/ErrorInterceptor.h>
#include <js/Initialization.h>
#include <js/Object.h>
#include <js/RootingAPI.h>
#include <js/SourceText.h>
#include <js/Value.h>

#include <format>

namespace glua::spidermonkey {
template <typename T>
struct converter {
};

template <decays_to<std::string> T>
struct converter<T> {
    static result<JS::Value> to_js(JSContext* cx, const std::string& v)
    {
        return JS::StringValue(JS_NewStringCopyN(cx, v.data(), v.size()));
    }

    static result<std::string> from_js(JSContext* cx, JS::HandleValue v)
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
};

template <decays_to<std::string_view> T>
struct converter<T> {
    static result<JS::Value> to_js(JSContext* cx, std::string_view v)
    {
        return JS::StringValue(JS_NewStringCopyN(cx, v.data(), v.size()));
    }

    // from_js not supported, can't 'point' to JS bytes
};

template <>
struct converter<int8_t> {
    static result<JS::Value> to_js(JSContext*, int8_t v)
    {
        JS::Value result;
        result.setNumber(v);
        return result;
    }

    static result<int8_t> from_js(JSContext* cx, JS::HandleValue v)
    {
        int8_t retval;
        JS::ToInt8(cx, v, &retval);

        return retval;
    }
};

template <>
struct converter<uint8_t> {
    static result<JS::Value> to_js(JSContext*, uint8_t v)
    {
        JS::Value result;
        result.setNumber(v);
        return result;
    }

    static result<uint8_t> from_js(JSContext* cx, JS::HandleValue v)
    {
        uint8_t retval;
        JS::ToUint8(cx, v, &retval);

        return retval;
    }
};

template <>
struct converter<int16_t> {
    static result<JS::Value> to_js(JSContext*, int16_t v)
    {
        JS::Value result;
        result.setNumber(v);
        return result;
    }

    static result<int16_t> from_js(JSContext* cx, JS::HandleValue v)
    {
        int16_t retval;
        JS::ToInt16(cx, v, &retval);

        return retval;
    }
};

template <>
struct converter<uint16_t> {
    static result<JS::Value> to_js(JSContext*, uint16_t v)
    {
        JS::Value result;
        result.setNumber(v);
        return result;
    }

    static result<uint16_t> from_js(JSContext* cx, JS::HandleValue v)
    {
        uint16_t retval;
        JS::ToUint16(cx, v, &retval);

        return retval;
    }
};

template <>
struct converter<int32_t> {
    static result<JS::Value> to_js(JSContext*, int32_t v) { return JS::Int32Value(v); }

    static result<int32_t> from_js(JSContext* cx, JS::HandleValue v)
    {
        int32_t retval;
        JS::ToInt32(cx, v, &retval);

        return retval;
    }
};

template <>
struct converter<uint32_t> {
    static result<JS::Value> to_js(JSContext*, uint32_t v)
    {
        JS::Value result;
        result.setNumber(v);
        return result;
    }

    static result<uint32_t> from_js(JSContext* cx, JS::HandleValue v)
    {
        uint32_t retval;
        JS::ToUint32(cx, v, &retval);

        return retval;
    }
};

template <>
struct converter<int64_t> {
    static result<JS::Value> to_js(JSContext*, int64_t v)
    {
        JS::Value result;
        result.setNumber(v);
        return result;
    }

    static result<int64_t> from_js(JSContext* cx, JS::HandleValue v)
    {
        int64_t retval;
        JS::ToInt64(cx, v, &retval);

        return retval;
    }
};

template <>
struct converter<uint64_t> {
    static result<JS::Value> to_js(JSContext*, uint64_t v)
    {
        JS::Value result;
        result.setNumber(v);
        return result;
    }

    static result<uint64_t> from_js(JSContext* cx, JS::HandleValue v)
    {
        uint64_t retval;
        JS::ToUint64(cx, v, &retval);

        return retval;
    }
};

template <>
struct converter<float> {
    static result<JS::Value> to_js(JSContext*, float v) { return JS::Float32Value(v); }

    static result<float> from_js(JSContext* cx, JS::HandleValue v)
    {
        double retval;
        JS::ToNumber(cx, v, &retval);

        return static_cast<float>(retval);
    }
};

template <>
struct converter<double> {
    static result<JS::Value> to_js(JSContext*, double v) { return JS::DoubleValue(v); }

    static result<double> from_js(JSContext* cx, JS::HandleValue v)
    {
        double retval;
        JS::ToNumber(cx, v, &retval);

        return retval;
    }
};

template <>
struct converter<bool> {
    static result<JS::Value> to_js(JSContext*, bool v) { return JS::BooleanValue(v); }

    static result<bool> from_js(JSContext*, JS::HandleValue v) { return JS::ToBoolean(v); }
};

template <decays_to_integral T>
struct converter<T> : converter<std::decay_t<T>> { };

template <typename T>
auto to_js(JSContext* cx, T&& value)
{
    return converter<T>::to_js(cx, static_cast<T&&>(value));
}

template <typename T>
auto from_js(JSContext* cx, JS::HandleValue v)
{
    return converter<T>::from_js(cx, v);
}

template <typename T>
using ValueFor = JS::Value;

template <typename... Ts>
auto many_to_js(JSContext* cx, Ts&&... values)
{
    auto many_expected = std::make_tuple(to_js(cx, static_cast<Ts&&>(values))...);
    return many_results_to_one(std::move(many_expected));
}

template <typename T>
using HandleValueFor = JS::HandleValue;

template <typename... Ts>
auto many_from_js([[maybe_unused]] JSContext* cx, HandleValueFor<Ts>... vs)
{
    auto many_expected = std::make_tuple(from_js<Ts>(cx, vs)...);
    return many_results_to_one(std::move(many_expected));
}

template <typename ReturnType, typename... ArgTypes>
bool call_generic_wrapped_functor(generic_functor<ReturnType, ArgTypes...>& f, JSContext* cx, JS::CallArgs& args);

template <typename ReturnType, typename ClassType, typename... ArgTypes>
bool call_generic_wrapped_method(generic_functor<ReturnType, ClassType, ArgTypes...>& f, JSContext* cx, JS::CallArgs& args);

template <registered_class T>
struct class_registration_impl {
    using registration = class_registration<T>;

    static result<T*> unwrap_object(JSContext*, JS::HandleValue v)
    {
        if (v.isObject()) {
            JSObject& obj = v.toObject();
            const JS::Value& reserved_value = JS::GetReservedSlot(&obj, SLOT_VALUE_MUTABLE);
            if (reserved_value.toBoolean()) {
                const JS::Value& reserved_value = JS::GetReservedSlot(&obj, SLOT_OBJECT_PTR);
                return reinterpret_cast<T*>(reserved_value.toPrivate());
            } else {
                return unexpected("unwrap failed - attempted to extract mutable reference to const object");
            }
        } else {
            return unexpected("unwrap on non-object value");
        }
    }

    static result<const T*> unwrap_object_const(JSContext*, JS::HandleValue v)
    {
        if (v.isObject()) {
            JSObject& obj = v.toObject();
            const JS::Value& reserved_value = JS::GetReservedSlot(&obj, SLOT_OBJECT_PTR);
            return reinterpret_cast<T*>(reserved_value.toPrivate());
        } else {
            return unexpected("unwrap (const) on non-object value");
        }
    }

    static result<JSObject*> wrap_object(JSContext* cx, T* obj_ptr, bool owned_by_js = false)
    {
        JS::RootedObject obj { cx, JS_NewObjectWithGivenProto(cx, &class_, proto_) };

        JS::SetReservedSlot(obj, SLOT_OBJECT_PTR, JS::PrivateValue(obj_ptr)); // release v here
        JS::SetReservedSlot(obj, SLOT_VALUE_OWNED_BY_JS, JS::BooleanValue(owned_by_js)); // whether owned in JS
        JS::SetReservedSlot(obj, SLOT_VALUE_MUTABLE, JS::BooleanValue(true)); // whether the value is mutable

        return obj;
    }

    static result<JSObject*> wrap_object(JSContext* cx, std::unique_ptr<T> obj_ptr)
    {
        return wrap_object(cx, obj_ptr.release(), true);
    }

    static result<JSObject*> wrap_object(JSContext* cx, std::unique_ptr<const T> obj_ptr)
    {
        return wrap_object(cx, obj_ptr.release(), true);
    }

    static result<JSObject*> wrap_object(JSContext* cx, const T* obj_ptr, bool owned_by_js = false)
    {
        JS::RootedObject obj { cx, JS_NewObjectWithGivenProto(cx, &class_, proto_) };

        // const_cast here is obviously a const violation, it means at runtime we're now responsible
        // for const checking
        JS::SetReservedSlot(obj, SLOT_OBJECT_PTR, JS::PrivateValue(const_cast<T*>(obj_ptr))); // release v here
        JS::SetReservedSlot(obj, SLOT_VALUE_OWNED_BY_JS, JS::BooleanValue(owned_by_js)); // whether owned in JS
        JS::SetReservedSlot(obj, SLOT_VALUE_MUTABLE, JS::BooleanValue(false)); // whether the value is mutable

        return obj;
    }

    static bool constructor(JSContext* cx, unsigned argc, JS::Value* vp)
    {
        auto args = JS::CallArgsFromVp(argc, vp);
        return call_generic_wrapped_functor(*registration::constructor, cx, args);
    }

    static void finalizer(JS::GCContext*, JSObject* obj)
    {
        const JS::Value& reserved_value_ptr = JS::GetReservedSlot(obj, SLOT_OBJECT_PTR);
        const JS::Value& reserved_value_owned = JS::GetReservedSlot(obj, SLOT_VALUE_OWNED_BY_JS);

        if (reserved_value_owned.toBoolean()) {
            std::unique_ptr<T> reacquired_ptr { reinterpret_cast<T*>(reserved_value_ptr.toPrivate()) };
        }
    }

    template <std::size_t MethodIndex>
    static bool method_call(JSContext* cx, unsigned argc, JS::Value* vp)
    {
        auto args = JS::CallArgsFromVp(argc, vp);
        auto& method_data = std::get<MethodIndex>(registration::methods);

        return call_generic_wrapped_method(*method_data.generic_functor_ptr_, cx, args);
    }

    template <std::size_t FieldIndex>
    static bool field_getter(JSContext* cx, unsigned argc, JS::Value* vp)
    {
        auto args = JS::CallArgsFromVp(argc, vp);
        auto& field_data = std::get<FieldIndex>(registration::fields);

        return [&]<typename FieldType>(FieldType T::*field_ptr) {
            return from_js<const T&>(cx, args.thisv()).and_then([&](const T& obj) {
                return to_js(cx, obj.*field_ptr).transform([&](auto value) { args.rval().set(value); });
            });
        }(field_data.field_ptr_)
                   .transform([]() { return true; })
                   .or_else([&](const auto& error) -> result<bool> {
                       JS_ReportErrorASCII(cx, "%s", error.data());
                       return false;
                   })
                   .value();
    }

    static bool illegal_field_setter(JSContext* cx, unsigned, JS::Value*)
    {
        JS_ReportErrorASCII(cx, "attempt to write value to const field");
        return false;
    }

    template <std::size_t FieldIndex>
    static bool field_setter(JSContext* cx, unsigned argc, JS::Value* vp)
    {
        auto args = JS::CallArgsFromVp(argc, vp);
        auto& field_data = std::get<FieldIndex>(registration::fields);

        return [&]<typename FieldType>(FieldType T::*field_ptr) {
            return from_js<T&>(cx, args.thisv()).and_then([&](T& obj) {
                return from_js<FieldType>(cx, args.get(0)).transform([&]<typename V>(V&& val) {
                    (obj.*field_ptr) = std::forward<V>(val);
                });
            });
        }(field_data.field_ptr_)
                   .transform([]() { return true; })
                   .or_else([&](const auto& error) -> result<bool> {
                       JS_ReportErrorASCII(cx, "%s", error.data());
                       return false;
                   })
                   .value();
    }

    template <std::size_t I, typename M>
    static JSFunctionSpec method_to_spec(bound_method<M>& method)
    {
        return JS_FN(
            method.name_.data(), method_call<I>, static_cast<uint16_t>(method.generic_functor_ptr_->num_args), 0);
    }

    template <std::size_t I, typename FieldType>
    static JSPropertySpec field_to_spec(bound_field<FieldType T::*>& field)
    {
        if constexpr (std::is_const_v<FieldType>) {
            // this may look backwards (having a setter), but otherwise spidermonkey won't raise an error
            // when a script assigns a const value, it will just silently do nothing
            // so instead we assign a setter which simply reports an error to spidermonkey
            return JS_PSGS(field.name_.data(), field_getter<I>, illegal_field_setter, JSPROP_PERMANENT | JSPROP_ENUMERATE);
        } else {
            return JS_PSGS(field.name_.data(), field_getter<I>, field_setter<I>, JSPROP_PERMANENT | JSPROP_ENUMERATE);
        }
    }

    static void do_registration(JSContext* cx, JS::HandleObject scope)
    {
        auto methods_array = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return std::array<JSFunctionSpec, num_methods + 1> {
                method_to_spec<Is>(std::get<Is>(registration::methods))..., JS_FS_END
            };
        }(std::make_index_sequence<num_methods> {});
        auto properties_array = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return std::array<JSPropertySpec, num_fields + 1> {
                field_to_spec<Is>(std::get<Is>(registration::fields))..., JS_PS_END
            };
        }(std::make_index_sequence<num_fields> {});
        proto_.init(
            cx,
            JS_InitClass(
                cx,
                scope,
                &class_,
                nullptr,
                registration::name.data(),
                &constructor,
                registration::constructor->num_args,
                properties_array.data(), // propertyspec - attributes
                methods_array.data(), // functionspec - methods
                nullptr, // static propertyspec
                nullptr // static functionspec
                ));
    }

    static void do_deregistration() { proto_.reset(); }

    using methods_tuple = decltype(registration::methods);
    static constexpr std::size_t num_methods = std::tuple_size_v<methods_tuple>;

    using fields_tuple = decltype(registration::fields);
    static constexpr std::size_t num_fields = std::tuple_size_v<fields_tuple>;

    static constexpr JSClassOps ops_ = []() {
        JSClassOps ops {};
        ops.construct = constructor;
        ops.finalize = finalizer;

        return ops;
    }();

    static constexpr std::size_t SLOT_OBJECT_PTR { 0 };
    static constexpr std::size_t SLOT_VALUE_OWNED_BY_JS { 1 };
    static constexpr std::size_t SLOT_VALUE_MUTABLE { 2 };
    static inline const JSClass class_ { registration::name.data(), JSCLASS_HAS_RESERVED_SLOTS(3), &ops_, nullptr, nullptr, nullptr };
    static inline JS::PersistentRootedObject proto_;
};

template <registered_class T>
struct converter<T*> {
    using impl = class_registration_impl<T>;

    static result<JS::Value> to_js(JSContext* cx, T* v)
    {
        return impl::wrap_object(cx, v).transform([](auto* obj) { return JS::ObjectValue(*obj); });
    }

    static result<T*> from_js(JSContext* cx, JS::HandleValue v) { return impl::unwrap_object(cx, v); }
};

template <registered_class T>
struct converter<const T*> {
    using impl = class_registration_impl<T>;

    static result<JS::Value> to_js(JSContext* cx, const T* v)
    {
        return impl::wrap_object(cx, v).transform([](auto* obj) { return JS::ObjectValue(*obj); });
    }

    static result<const T*> from_js(JSContext* cx, JS::HandleValue v) { return impl::unwrap_object_const(cx, v); }
};

template <registered_class T>
struct converter<std::unique_ptr<T>> {
    using impl = class_registration_impl<T>;

    static result<JS::Value> to_js(JSContext* cx, std::unique_ptr<T> v)
    {
        return impl::wrap_object(cx, std::move(v)).transform([](auto* obj) { return JS::ObjectValue(*obj); });
    }
};

template <registered_class T>
struct converter<std::unique_ptr<const T>> {
    using impl = class_registration_impl<T>;

    static result<JS::Value> to_js(JSContext* cx, std::unique_ptr<const T> v)
    {
        return impl::wrap_object(cx, std::move(v)).transform([](auto* obj) { return JS::ObjectValue(*obj); });
    }
};

template <registered_class T>
struct converter<T&> {
    using impl = class_registration_impl<T>;

    static result<JS::Value> to_js(JSContext* cx, T& v)
    {
        return impl::wrap_object(cx, &v).transform([](auto* obj) { return JS::ObjectValue(*obj); });
    }

    static result<std::reference_wrapper<T>> from_js(JSContext* cx, JS::HandleValue v)
    {
        return impl::unwrap_object(cx, v).transform([](auto* ptr) { return std::ref(*ptr); });
    }
};

template <registered_class T>
struct converter<const T&> {
    using impl = class_registration_impl<T>;

    static result<JS::Value> to_js(JSContext* cx, const T& v)
    {
        return impl::wrap_object(cx, &v).transform([](auto* obj) { return JS::ObjectValue(*obj); });
    }

    static result<std::reference_wrapper<const T>> from_js(JSContext* cx, JS::HandleValue v)
    {
        return impl::unwrap_object_const(cx, v).transform([](const T* ptr) { return std::cref(*ptr); });
    }
};

template <registered_class T>
struct converter<std::reference_wrapper<T>> {
    using impl = class_registration_impl<T>;

    static result<JS::Value> to_js(JSContext* cx, std::reference_wrapper<T> v)
    {
        return impl::wrap_object(cx, &v.get()).transform([](auto* obj) { return JS::ObjectValue(*obj); });
    }

    static result<std::reference_wrapper<T>> from_js(JSContext* cx, JS::HandleValue v)
    {
        return impl::unwrap_object(cx, v).transform([](auto* ptr) { return std::ref(*ptr); });
    }
};

template <registered_class T>
struct converter<std::reference_wrapper<const T>> {
    using impl = class_registration_impl<T>;

    static result<JS::Value> to_js(JSContext* cx, std::reference_wrapper<const T> v)
    {
        return impl::wrap_object(cx, &v.get()).transform([](auto* obj) { return JS::ObjectValue(*obj); });
    }

    static result<std::reference_wrapper<const T>> from_js(JSContext* cx, JS::HandleValue v)
    {
        return impl::unwrap_object_const(cx, v).transform([](const T* ptr) { return std::cref(*ptr); });
    }
};

template <typename ReturnType, typename... ArgTypes>
bool call_generic_wrapped_functor(generic_functor<ReturnType, ArgTypes...>& f, JSContext* cx, JS::CallArgs& args)
{
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return many_from_js<ArgTypes...>(cx, args.get(Is)...).and_then([&](auto actual_args) -> result<void> {
            if constexpr (std::same_as<ReturnType, void>) {
                std::apply(
                    [&](auto&&... unwrapped_args) -> ReturnType {
                        f.call(std::forward<decltype(unwrapped_args)>(unwrapped_args)...);
                    },
                    std::move(actual_args));

                return {};
            } else {
                return to_js(
                    cx,
                    std::apply(
                        [&](auto&&... unwrapped_args) -> ReturnType {
                            return f.call(std::forward<decltype(unwrapped_args)>(unwrapped_args)...);
                        },
                        std::move(actual_args)))
                    .transform([&](auto v) { args.rval().set(v); });
            }
        });
    }(std::index_sequence_for<ArgTypes...> {})
               .transform([]() { return true; })
               .or_else([&](const auto& error) -> result<bool> {
                   JS_ReportErrorASCII(cx, "%s", error.data());
                   return false;
               })
               .value();
}

template <typename ReturnType, typename ClassType, typename... ArgTypes>
bool call_generic_wrapped_method(generic_functor<ReturnType, ClassType, ArgTypes...>& f, JSContext* cx, JS::CallArgs& args)
{
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return many_from_js<ClassType, ArgTypes...>(cx, args.thisv(), args.get(Is)...).and_then([&](auto actual_args) -> result<void> {
            if constexpr (std::same_as<ReturnType, void>) {
                std::apply(
                    [&](auto&&... unwrapped_args) -> ReturnType {
                        f.call(std::forward<decltype(unwrapped_args)>(unwrapped_args)...);
                    },
                    std::move(actual_args));

                return {};
            } else {
                return to_js(
                    cx,
                    std::apply(
                        [&](auto&&... unwrapped_args) -> ReturnType {
                            return f.call(std::forward<decltype(unwrapped_args)>(unwrapped_args)...);
                        },
                        std::move(actual_args)))
                    .transform([&](auto v) { args.rval().set(v); });
            }
        });
    }(std::index_sequence_for<ArgTypes...> {})
               .transform([]() { return true; })
               .or_else([&](const auto& error) -> result<bool> {
                   JS_ReportErrorASCII(cx, "%s", error.data());
                   return false;
               })
               .value();
}

template <typename ReturnType, typename... ArgTypes>
bool generic_wrapped_functor(JSContext* cx, unsigned argc, JS::Value* vp)
{
    if (argc != sizeof...(ArgTypes)) {
        JS_ReportErrorASCII(cx, "Invalid number of arguments");
        return false;
    }

    auto args = JS::CallArgsFromVp(argc, vp);
    JSObject& func = args.callee(); // actual functor should be stored here

    const JS::Value& v = js::GetFunctionNativeReserved(&func, 0);
    auto* functor = reinterpret_cast<generic_functor<ReturnType, ArgTypes...>*>(v.toPrivate());

    return call_generic_wrapped_functor(*functor, cx, args);
}

template <typename ReturnType, typename... ArgTypes>
JSNative callback_for(const generic_functor<ReturnType, ArgTypes...>&)
{
    return &generic_wrapped_functor<ReturnType, ArgTypes...>;
}

class backend {
public:
    static result<std::unique_ptr<backend>> create()
    {
        return do_global_init().and_then([&]() {
            return create_context().and_then([&](auto context) {
                return create_global_scope(context.value_).transform([&](auto* global_scope) {
                    return std::unique_ptr<backend> { new backend { std::move(context), global_scope } };
                });
            });
        });
    }

    template <typename ReturnType>
    result<ReturnType> execute_script(const std::string& code)
    {
        JS::SourceText<mozilla::Utf8Unit> source;
        if (!source.init(cx_.value_, code.data(), code.size(), JS::SourceOwnership::Borrowed)) {
            return unexpected("Spidermonkey failed to init source\n");
        }

        JS::CompileOptions compile_options { cx_.value_ };
        compile_options.setFileAndLine("inline", 1);

        JS::RootedScript compiled_script { cx_.value_, JS::Compile(cx_.value_, compile_options, source) };
        if (compiled_script == nullptr) {
            return unexpected("Spidermonkey failed to compile script\n");
        }

        JS::RootedValue return_value { cx_.value_ };
        if (!JS_ExecuteScript(cx_.value_, compiled_script, &return_value)) {
            return unexpected("Spidermonkey failed to execute script\n");
        }

        if constexpr (!std::same_as<ReturnType, void>) {
            return from_js<ReturnType>(cx_.value_, return_value);
        }
    }

    template <typename ReturnType, typename... ArgTypes>
    result<void> register_functor(const std::string& name, generic_functor<ReturnType, ArgTypes...>& functor)
    {
        JS::RootedFunction js_func {
            cx_.value_,
            js::DefineFunctionWithReserved(
                cx_.value_, global_scope_, name.data(), callback_for(functor), functor.num_args, 0)
        };
        if (js_func == nullptr) {
            return unexpected(std::format("Spidermonkey failed to define function"));
        }

        auto* obj = JS_GetFunctionObject(js_func);
        js::SetFunctionNativeReserved(obj, 0, JS::PrivateValue(&functor));

        return {};
    }

    template <typename T>
    result<T> get_global(const std::string& name)
    {
        JS::RootedValue prop { cx_.value_ };
        bool prop_found { false };
        if (JS_HasProperty(cx_.value_, global_scope_, name.data(), &prop_found) && prop_found && JS_GetProperty(cx_.value_, global_scope_, name.data(), &prop)) {
            return from_js<T>(cx_.value_, prop);
        }

        return unexpected(std::format("No global property with the name {} was found", name));
    }

    template <typename T>
    result<void> set_global(const std::string& name, T value)
    {
        return to_js(cx_.value_, std::move(value)).and_then([&](auto v) -> result<void> {
            JS::RootedValue value_handle { cx_.value_, v };
            if (!JS_SetProperty(cx_.value_, global_scope_, name.data(), value_handle)) {
                return unexpected(std::format("Spidermonkey failed to set {} global", name));
            }
            return {};
        });
    }

    template <typename ReturnType, typename... Args>
    result<ReturnType> call_function(const std::string& name, Args&&... args)
    {
        return many_to_js(cx_.value_, std::forward<Args>(args)...).and_then([&](auto actual_args) -> result<ReturnType> {
            JS::RootedValue call_return { cx_.value_ };

            JS::RootedValueArray<sizeof...(args)> rooted_args {
                cx_.value_,
                std::apply(
                    [&](auto&&... unwrapped_args) {
                        return JS::ValueArray<sizeof...(unwrapped_args)> {
                            std::forward<decltype(unwrapped_args)>(unwrapped_args)...
                        };
                    },
                    std::move(actual_args))
            };
            JS::HandleValueArray call_args { rooted_args };
            if (!JS_CallFunctionName(cx_.value_, global_scope_, name.data(), call_args, &call_return)) {
                return unexpected(std::format("Spidermonkey failed to call function with name {}", name));
            }

            if constexpr (!std::same_as<ReturnType, void>) {
                return from_js<ReturnType>(cx_.value_, call_return);
            }
        });
    }

    template <registered_class T>
    void register_class()
    {
        class_registration_impl<T>::do_registration(cx_.value_, global_scope_);
    }

private:
    struct global_init {
        global_init(const global_init&) = delete;
        global_init& operator=(const global_init&) = delete;
        global_init(global_init&& move)
            : responsible_(std::exchange(move.responsible_, false))
        {
        }
        global_init& operator=(global_init&& move)
        {
            if (this != &move) {
                if (responsible_) {
                    JS_ShutDown();
                }
                responsible_ = std::exchange(move.responsible_, false);
            }
            return *this;
        }

        global_init(bool success)
            : responsible_(success)
        {
        }
        ~global_init()
        {
            if (responsible_)
                JS_ShutDown();
        }

        bool responsible_;
    };

    struct context {
        context(const context&) = delete;
        context& operator=(const context&) = delete;
        context(context&& move)
            : value_(std::exchange(move.value_, nullptr))
        {
        }
        context& operator=(context&& move)
        {
            if (this != &move) {
                if (value_)
                    JS_DestroyContext(value_);
                value_ = std::exchange(move.value_, nullptr);
            }
            return *this;
        }

        context(JSContext* value)
            : value_(value)
        {
        }
        ~context()
        {
            if (value_)
                JS_DestroyContext(value_);
        }

        JSContext* value_;
    };

    static result<void> do_global_init()
    {
        static bool result { JS_Init() };
        static global_init init { result };
        if (!result) {
            return unexpected("Spidermonkey failed to init (JS_Init)");
        }

        return {};
    }

    static result<context> create_context()
    {
        JSContext* value = JS_NewContext(JS::DefaultHeapMaxBytes);
        if (value) {
            return context { value };
        } else {
            return unexpected("Spidermonkey failed to create context (JS_NewContext)");
        }
    }

    static result<JSObject*> create_global_scope(JSContext* cx)
    {
        if (!JS::InitSelfHostedCode(cx)) {
            return unexpected("Spidermonkey error initializing self hosted code");
        }

        static JSClass global_object { "GlobalObject", JSCLASS_GLOBAL_FLAGS, &JS::DefaultGlobalClassOps, nullptr, nullptr, nullptr };
        JS::RealmOptions options;
        JSObject* result = JS_NewGlobalObject(cx, &global_object, nullptr, JS::FireOnNewGlobalHook, options);
        if (result == nullptr) {
            return unexpected("Spidermonkey error creating global scope");
        }

        return result;
    }

    backend(context cx, JSObject* global_scope)
        : cx_(std::move(cx))
        , global_scope_(cx_.value_, global_scope)
        , auto_realm_(cx_.value_, global_scope_.get())
    {
    }

    context cx_;
    JS::RootedObject global_scope_;
    JSAutoRealm auto_realm_;
};

} // namespace spidermonkey
