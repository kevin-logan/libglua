#pragma once

// NOTE: Do not include this, include glua/backends/spidermonkey.hpp instead, the include order is carefully
// crafted to separate declarations and dependent definitions

namespace glua::spidermonkey {
struct any_spidermonkey_impl : any_impl {
    virtual result<JS::Value> to_js(JSContext* cx) && = 0;
};

struct any_basic_impl final : any_spidermonkey_impl {
    template <decays_to_basic_type T>
    any_basic_impl(T&& value)
        : value_(std::in_place_type_t<std::decay_t<T>> {}, std::forward<T>(value))
    {
    }

    result<JS::Value> to_js(JSContext* cx) && override
    {
        return std::visit(
            [&]<typename T>(T&& value) {
                return spidermonkey::to_js(cx, std::forward<T>(value));
            },
            std::move(value_));
    }

    basic_type_variant value_;
};

template <typename T>
struct any_array_impl final : any_spidermonkey_impl {
    any_array_impl(std::vector<T> value)
        : value_(std::move(value))
    {
    }

    result<JS::Value> to_js(JSContext* cx) && override
    {
        return spidermonkey::to_js(cx, std::move(value_));
    }

    std::vector<T> value_;
};

template <typename T>
struct any_map_impl final : any_spidermonkey_impl {
    any_map_impl(std::unordered_map<std::string, T> value)
        : value_(std::move(value))
    {
    }

    result<JS::Value> to_js(JSContext* cx) && override
    {
        return spidermonkey::to_js(cx, std::move(value_));
    }

    std::unordered_map<std::string, T> value_;
};

struct any_registered_class_impl final : any_spidermonkey_impl {
    any_registered_class_impl(JS::HandleObject obj, JSObject* proto)
        : value_(JS::GetReservedSlot(obj, SLOT_OBJECT_PTR).toPrivate())
        , class_(JS::GetClass(obj))
        , proto_(proto)
        , mutable_(JS::GetReservedSlot(obj, SLOT_VALUE_MUTABLE).toBoolean())
        , has_ownership_(JS::GetReservedSlot(obj, SLOT_VALUE_OWNED_BY_JS).toBoolean())
        , moved_out_(false)
        , finalizer_(reinterpret_cast<registered_class_finalizer>(JS::GetReservedSlot(obj, SLOT_FINALIZER).toPrivate()))
    {
        // un-own it, temporary, this is a leak
        JS::SetReservedSlot(obj, SLOT_VALUE_OWNED_BY_JS, JS::BooleanValue(false));
    }

    result<JS::Value> to_js(JSContext* cx) && override
    {
        if (!std::exchange(moved_out_, true)) {
            JS::RootedObject proto_handle { cx, proto_ };
            JS::RootedObject obj { cx, JS_NewObjectWithGivenProto(cx, class_, proto_handle) };

            if (obj == nullptr)
                return unexpected("Could not recreate object from any");

            // this ownership is extremely complicated and user error can result in double-frees
            // or memory leaks. If JS owns the value and passes it to a function, we should borrow
            // it, not take ownership (as the JS value still holds it). But if JS returns the value
            // then if we borrow it we will soon have a dangling reference.
            // The current decision is to always borrow, which makes JS responsible for keeping objects
            // around that are still referenced in C++
            JS::SetReservedSlot(obj, SLOT_OBJECT_PTR, JS::PrivateValue(value_));
            JS::SetReservedSlot(obj, SLOT_VALUE_OWNED_BY_JS, JS::BooleanValue(has_ownership_));
            JS::SetReservedSlot(obj, SLOT_VALUE_MUTABLE, JS::BooleanValue(mutable_));
            JS::SetReservedSlot(obj, SLOT_FINALIZER, JS::PrivateValue(reinterpret_cast<void*>(finalizer_)));

            return JS::ObjectValue(*obj);
        } else {
            return unexpected("any object consumed more than once");
        }
    }

    ~any_registered_class_impl() override
    {
        // if we were never moved out and we have ownership we must destroy the data, which requires
        // some trickery as we've lost the type, and we don't have a JS context with which to recreate
        // the object (and let it destruct using the JS finalizer), so there's a special slot on the
        // objects which stores a type-erased finalizer for just this purpose
        if (!moved_out_ && has_ownership_)
            finalizer_(value_);
    }

    void* value_;
    const JSClass* class_;
    JSObject* proto_;
    bool mutable_;
    bool has_ownership_;
    bool moved_out_;
    registered_class_finalizer finalizer_;
};
}
