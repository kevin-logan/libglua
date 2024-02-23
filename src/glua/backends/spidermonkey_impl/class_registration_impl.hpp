#pragma once

// NOTE: Do not include this, include glua/backends/spidermonkey.hpp instead, the include order is carefully
// crafted to separate declarations and dependent definitions

namespace glua::spidermonkey {
constexpr std::size_t SLOT_OBJECT_PTR { 0 };
constexpr std::size_t SLOT_VALUE_OWNED_BY_JS { 1 };
constexpr std::size_t SLOT_VALUE_MUTABLE { 2 };
using registered_class_finalizer = void (*)(void*);
constexpr std::size_t SLOT_FINALIZER { 3 };
constexpr std::size_t SLOT_COUNT { 4 };

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
        JS::SetReservedSlot(obj, SLOT_FINALIZER, JS::PrivateValue(reinterpret_cast<void*>(finalizer_vp))); // finalizer for if we lose type information (such as in any destructor)

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
        JS::SetReservedSlot(obj, SLOT_FINALIZER, JS::PrivateValue(reinterpret_cast<void*>(finalizer_vp))); // finalizer for if we lose type information (such as in any destructor)

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

    static void finalizer_vp(void* data_ptr)
    {
        std::unique_ptr<T> reacquired_ptr { reinterpret_cast<T*>(data_ptr) };
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

    static inline const JSClass class_ { registration::name.data(), JSCLASS_HAS_RESERVED_SLOTS(SLOT_COUNT), &ops_, nullptr, nullptr, nullptr };
    static inline JS::PersistentRootedObject proto_;
};
}