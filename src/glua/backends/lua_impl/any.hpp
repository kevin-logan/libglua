#pragma once

// NOTE: Do not include this, include glua/backends/lua.hpp instead, the include order is carefully
// crafted to separate declarations and dependent definitions

namespace glua::lua {
struct any_lua_impl : any_impl {
    virtual result<void> push_to_lua(lua_State* lua) && = 0;
};

struct any_basic_impl final : any_lua_impl {
    template <decays_to_basic_type T>
    any_basic_impl(T&& value)
        : value_(std::in_place_type_t<std::decay_t<T>> {}, std::forward<T>(value))
    {
    }

    result<void> push_to_lua(lua_State* lua) && override
    {
        return std::visit(
            [&]<typename T>(T&& value) {
                return lua::push_to_lua(lua, std::forward<T>(value));
            },
            std::move(value_));
    }

    basic_type_variant value_;
};

template <typename T>
struct any_array_impl final : any_lua_impl {
    any_array_impl(std::vector<T> value)
        : value_(std::move(value))
    {
    }

    result<void> push_to_lua(lua_State* lua) && override
    {
        return lua::push_to_lua(lua, std::move(value_));
    }

    std::vector<T> value_;
};

template <typename T>
struct any_map_impl final : any_lua_impl {
    any_map_impl(std::unordered_map<std::string, T> value)
        : value_(std::move(value))
    {
    }

    result<void> push_to_lua(lua_State* lua) && override
    {
        return lua::push_to_lua(lua, std::move(value_));
    }

    std::unordered_map<std::string, T> value_;
};

struct any_registered_class_impl final : any_lua_impl {
    any_registered_class_impl(class_registration_data* value)
        : data_copy_(*value)
        , moved_out_(false)
    {
        value->owned_by_lua_ = false;
    }

    result<void> push_to_lua(lua_State* lua) && override
    {
        if (!std::exchange(moved_out_, true)) {
            class_registration_data* data = static_cast<class_registration_data*>(lua_newuserdata(lua, sizeof(class_registration_data)));
            new (data) class_registration_data { data_copy_ };

            // set the metatable for this object
            luaL_getmetatable(lua, data->registration_name_);
            lua_setmetatable(lua, -2);

            return {};
        } else {
            return unexpected("any object consumed more than once");
        }
    }

    ~any_registered_class_impl() override
    {
        // if we were never moved out and we have ownership we must destroy the data, which requires
        // some trickery as we've lost the type, and we don't have a lua state with which to recreate
        // the object (and let it destruct using lua), so there's a special value in the registration
        // which stores a type-erased finalizer for just this purpose
        if (!moved_out_ && data_copy_.owned_by_lua_)
            data_copy_.finalizer_(data_copy_.ptr_);
    }

    class_registration_data data_copy_;
    bool moved_out_;
};
}
