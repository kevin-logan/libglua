#pragma once

// NOTE: Do not include this, include glua/backends/lua.hpp instead, the include order is carefully
// crafted to separate declarations and dependent definitions

namespace glua::lua {
struct any_lua_impl : any_impl {
    virtual result<void> push_to_lua(lua_State* lua) const = 0;
};

struct any_basic_impl final : any_lua_impl {
    template <decays_to_basic_type T>
    any_basic_impl(T&& value)
        : value_(std::in_place_type_t<std::decay_t<T>> {}, std::forward<T>(value))
    {
    }

    result<void> push_to_lua(lua_State* lua) const override
    {
        return std::visit(
            [&]<typename T>(const T& value) {
                return lua::push_to_lua(lua, value);
            },
            value_);
    }

    basic_type_variant value_;
};

template <typename T>
struct any_array_impl final : any_lua_impl {
    any_array_impl(std::vector<T> value)
        : value_(std::move(value))
    {
    }

    result<void> push_to_lua(lua_State* lua) const override
    {
        return lua::push_to_lua(lua, value_);
    }

    std::vector<T> value_;
};

template <typename T>
struct any_map_impl final : any_lua_impl {
    any_map_impl(std::unordered_map<std::string, T> value)
        : value_(std::move(value))
    {
    }

    result<void> push_to_lua(lua_State* lua) const override
    {
        return lua::push_to_lua(lua, value_);
    }

    std::unordered_map<std::string, T> value_;
};
}
