#pragma once

// NOTE: Do not include this, include glua/backends/spidermonkey.hpp instead, the include order is carefully
// crafted to separate declarations and dependent definitions

namespace glua::spidermonkey {
struct any_spidermonkey_impl : any_impl {
    virtual result<JS::Value> to_js(JSContext* cx) const = 0;
};

struct any_basic_impl final : any_spidermonkey_impl {
    template <decays_to_basic_type T>
    any_basic_impl(T&& value)
        : value_(std::in_place_type_t<std::decay_t<T>> {}, std::forward<T>(value))
    {
    }

    result<JS::Value> to_js(JSContext* cx) const override
    {
        return std::visit(
            [&]<typename T>(const T& value) {
                return spidermonkey::to_js(cx, value);
            },
            value_);
    }

    basic_type_variant value_;
};

template <typename T>
struct any_array_impl final : any_spidermonkey_impl {
    any_array_impl(std::vector<T> value)
        : value_(std::move(value))
    {
    }

    result<JS::Value> to_js(JSContext* cx) const override
    {
        return spidermonkey::to_js(cx, value_);
    }

    std::vector<T> value_;
};

template <typename T>
struct any_map_impl final : any_spidermonkey_impl {
    any_map_impl(std::unordered_map<std::string, T> value)
        : value_(std::move(value))
    {
    }

    result<JS::Value> to_js(JSContext* cx) const override
    {
        return spidermonkey::to_js(cx, value_);
    }

    std::unordered_map<std::string, T> value_;
};
}
