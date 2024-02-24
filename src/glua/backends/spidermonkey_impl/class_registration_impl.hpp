#pragma once

// NOTE: Do not include this, include glua/backends/spidermonkey.hpp instead, the include order is carefully
// crafted to separate declarations and dependent definitions

namespace glua::spidermonkey {
constexpr std::size_t SLOT_OBJECT_PTR { 0 };
constexpr std::size_t SLOT_COUNT { 1 };

struct class_registration_data;
using class_registration_data_ptr = std::shared_ptr<class_registration_data>;

using registered_class_finalizer = void (*)(void*);
using to_any = std::unique_ptr<any_impl> (*)(class_registration_data_ptr*);

struct class_registration_data {
    class_registration_data(void* ptr, bool owned_by_js, bool is_mutable, to_any make_any, registered_class_finalizer f)
        : ptr_(ptr)
        , owned_by_js_(owned_by_js)
        , mutable_(is_mutable)
        , make_any_(make_any)
        , finalizer_(f)
    {
    }

    ~class_registration_data()
    {
        // this class is stored in a shared_ptr, when it finally destroys we may own the underlying data and need to destroy that
        if (owned_by_js_)
            finalizer_(ptr_);
    }

    void* ptr_;
    bool owned_by_js_;
    bool mutable_;
    to_any make_any_;
    registered_class_finalizer finalizer_;
};

template <registered_class T>
struct class_registration_impl {
    using registration = class_registration<T>;

    static std::unique_ptr<any_impl> make_any(class_registration_data_ptr* obj)
    {
        struct any_registered_class_impl : any_spidermonkey_impl {
            any_registered_class_impl(class_registration_data_ptr value)
                : value_(std::move(value))
            {
            }

            result<JS::Value> to_js(JSContext* cx) const override
            {
                JS::RootedObject obj { cx, JS_NewObjectWithGivenProto(cx, &class_, proto_) };

                // new shared_ptr*, copying existing shared_ptr
                auto* wrapper_ptr = new class_registration_data_ptr { value_ };

                JS::SetReservedSlot(obj, SLOT_OBJECT_PTR, JS::PrivateValue(wrapper_ptr));

                return JS::ObjectValue(*obj);
            }

            class_registration_data_ptr value_;
        };

        return std::make_unique<any_registered_class_impl>(*obj); // copy shared ownership here
    }

    static result<T*> unwrap_object(JSContext*, JS::HandleValue v)
    {
        if (v.isObject()) {
            JSObject& obj = v.toObject();
            const JS::Value& reserved_value = JS::GetReservedSlot(&obj, SLOT_OBJECT_PTR);
            auto* data = reinterpret_cast<class_registration_data_ptr*>(reserved_value.toPrivate())->get();

            if (data->mutable_)
                return reinterpret_cast<T*>(data->ptr_);
            else
                return unexpected("unwrap failed - attempted to extract mutable reference to const object");
        } else {
            return unexpected("unwrap on non-object value");
        }
    }

    static result<const T*> unwrap_object_const(JSContext*, JS::HandleValue v)
    {
        if (v.isObject()) {
            JSObject& obj = v.toObject();
            const JS::Value& reserved_value = JS::GetReservedSlot(&obj, SLOT_OBJECT_PTR);
            auto* data = reinterpret_cast<class_registration_data_ptr*>(reserved_value.toPrivate())->get();
            return reinterpret_cast<T*>(data->ptr_);
        } else {
            return unexpected("unwrap (const) on non-object value");
        }
    }

    static result<JSObject*> wrap_object(JSContext* cx, T* obj_ptr, bool owned_by_js = false)
    {
        JS::RootedObject obj { cx, JS_NewObjectWithGivenProto(cx, &class_, proto_) };

        auto* wrapper_ptr = new class_registration_data_ptr {
            std::make_shared<class_registration_data>(obj_ptr, owned_by_js, true, make_any, finalizer_vp)
        };

        JS::SetReservedSlot(obj, SLOT_OBJECT_PTR, JS::PrivateValue(wrapper_ptr));

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
        auto* wrapper_ptr = new class_registration_data_ptr {
            std::make_shared<class_registration_data>(const_cast<T*>(obj_ptr), owned_by_js, false, make_any, finalizer_vp)
        };

        JS::SetReservedSlot(obj, SLOT_OBJECT_PTR, JS::PrivateValue(wrapper_ptr));

        return obj;
    }

    static bool constructor(JSContext* cx, unsigned argc, JS::Value* vp)
    {
        auto args = JS::CallArgsFromVp(argc, vp);
        return call_generic_wrapped_functor(*registration::constructor, cx, args);
    }

    static void finalizer(JS::GCContext*, JSObject* obj)
    {
        const JS::Value& reserved_value = JS::GetReservedSlot(obj, SLOT_OBJECT_PTR);
        auto* data = reinterpret_cast<class_registration_data_ptr*>(reserved_value.toPrivate());

        // this deletes the shared_ptr<class_registration_data>, which may or may not delete
        // the actual T* within, depending on owned_by_js_ value
        // NOTE: data may be nullptr (in the case of the proto_ object)
        delete data;
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
        // proto will eventually be collected, and the finalizer will be called, so we must
        // set the slot to nullptr so the finalizer does nothing
        JS::SetReservedSlot(proto_, SLOT_OBJECT_PTR, JS::PrivateValue(nullptr));
    }

    static void do_deregistration()
    {
        proto_.reset();
    }

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