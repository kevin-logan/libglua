#pragma once

// NOTE: Do not include this, include glua/backends/lua.hpp instead, the include order is carefully
// crafted to separate declarations and dependent definitions

namespace glua::lua {

struct class_registration_data;
using class_registration_data_ptr = std::shared_ptr<class_registration_data>;

using registered_class_finalizer = void (*)(void*);
using to_any = std::unique_ptr<any_impl> (*)(class_registration_data_ptr*);

struct class_registration_data {
    class_registration_data(void* ptr, bool owned_by_lua, bool is_mutable, to_any make_any, registered_class_finalizer finalizer)
        : ptr_(ptr)
        , owned_by_lua_(owned_by_lua)
        , mutable_(is_mutable)
        , make_any_(make_any)
        , finalizer_(finalizer)
    {
    }

    ~class_registration_data()
    {
        // this class is stored in a shared_ptr, when it finally destroys we may own the underlying data and need to destroy that
        if (owned_by_lua_)
            finalizer_(ptr_);
    }

    void* ptr_;
    bool owned_by_lua_;
    bool mutable_;
    to_any make_any_;
    registered_class_finalizer finalizer_;
};

template <registered_class T>
struct class_registration_impl {
    using registration = class_registration<T>;

    // definition before any conversions
    static std::unique_ptr<any_impl> make_any(class_registration_data_ptr* obj)
    {
        struct any_registered_class_impl : any_lua_impl {
            any_registered_class_impl(class_registration_data_ptr value)
                : value_(std::move(value))
            {
            }

            result<void> push_to_lua(lua_State* lua) const override
            {
                auto* data = static_cast<class_registration_data_ptr*>(lua_newuserdata(lua, sizeof(class_registration_data_ptr)));

                // placement new needs with a a copy of the original shared_ptr
                new (data) class_registration_data_ptr { value_ };

                // set the metatable for this object
                luaL_getmetatable(lua, registration::name.data());
                lua_setmetatable(lua, -2);

                return {};
            }

            class_registration_data_ptr value_;
        };

        return std::make_unique<any_registered_class_impl>(*obj); // copy shared ownership here
    }

    static result<T*> unwrap_object(lua_State* lua, int i)
    {
        if (lua_isuserdata(lua, i)) {
            auto* data = static_cast<class_registration_data_ptr*>(lua_touserdata(lua, i))->get();
            if (data->mutable_) {
                return static_cast<T*>(data->ptr_);
            } else {
                return unexpected("unwrap failed - attempted to extract mutable reference to const object");
            }
        } else {
            return unexpected("unwrap on non-object value");
        }
    }

    static result<const T*> unwrap_object_const(lua_State* lua, int i)
    {
        if (lua_isuserdata(lua, i)) {
            auto* data = static_cast<class_registration_data_ptr*>(lua_touserdata(lua, i))->get();
            return static_cast<const T*>(data->ptr_);
        } else {
            return unexpected("unwrap on non-object value");
        }
    }

    static result<void> push_wrapped_object(lua_State* lua, T* obj_ptr, bool owned_by_lua = false)
    {
        auto* data = static_cast<class_registration_data_ptr*>(lua_newuserdata(lua, sizeof(class_registration_data_ptr)));
        new (data) class_registration_data_ptr { std::make_shared<class_registration_data>(obj_ptr, owned_by_lua, true, make_any, destructor_vp) };

        // set the metatable for this object
        luaL_getmetatable(lua, registration::name.data());
        lua_setmetatable(lua, -2);

        return {};
    }

    static result<void> push_wrapped_object(lua_State* lua, std::unique_ptr<T> obj_ptr)
    {
        return push_wrapped_object(lua, obj_ptr.release(), true);
    }

    static result<void> push_wrapped_object(lua_State* lua, const T* obj_ptr, bool owned_by_lua = false)
    {
        auto* data = static_cast<class_registration_data_ptr*>(lua_newuserdata(lua, sizeof(class_registration_data_ptr)));

        // const_cast here is obviously a const violation, it means at runtime we're now responsible
        // for const checking
        new (data) class_registration_data_ptr { std::make_shared<class_registration_data>(const_cast<T*>(obj_ptr), owned_by_lua, false, make_any, destructor_vp) };

        // set the metatable for this object
        luaL_getmetatable(lua, registration::name.data());
        lua_setmetatable(lua, -2);

        return {};
    }

    static result<void> push_wrapped_object(lua_State* lua, std::unique_ptr<const T> obj_ptr)
    {
        return push_wrapped_object(lua, obj_ptr.release(), true);
    }

    static int constructor(lua_State* lua)
    {
        return call_generic_wrapped_functor(*registration::constructor, lua);
    }

    static int destructor(lua_State* lua)
    {
        auto* data = static_cast<class_registration_data_ptr*>(lua_touserdata(lua, 1));

        // then we need to destroy the data
        std::destroy_at(data); // destroy shared pointer, may or may not actually own

        return 0;
    }

    static void destructor_vp(void* data_ptr)
    {
        std::unique_ptr<T> reacquired_ptr { reinterpret_cast<T*>(data_ptr) };
    }

    template <std::size_t MethodIndex>
    static int method_call(lua_State* lua)
    {
        return call_generic_wrapped_functor(*std::get<MethodIndex>(registration::methods).generic_functor_ptr_, lua);
    }

    template <std::size_t MethodIndex>
    static int method_index_handler(lua_State* lua)
    {
        // stack: 1. this 2. key
        lua_pushcfunction(lua, method_call<MethodIndex>);
        return 1;
    }

    template <std::size_t FieldIndex>
    static int field_index_handler(lua_State* lua)
    {
        // stack: 1. this 2. key
        return [&]<typename V>(bound_field<V T::*>& field) {
            return from_lua<T&>(lua, 1).and_then([&](T& self) -> result<int> {
                return push_to_lua(lua, self.*field.field_ptr_).transform([&]() { return 1; }); // 1 item pushed to stack
            });
        }(std::get<FieldIndex>(registration::fields))
                   .or_else([&](const std::string& error) -> result<int> {
                       lua_pushstring(lua, error.data());
                       lua_error(lua);

                       return unexpected("lua: unreachable post lua_error code reached");
                   })
                   .value();
    }

    template <std::size_t FieldIndex>
    static int field_newindex_handler(lua_State* lua)
    {
        // stack: 1. this 2. key 3. value
        return [&]<typename V>(bound_field<V T::*>& field) {
            return from_lua<T&>(lua, 1).and_then([&](T& self) -> result<int> {
                return from_lua<V>(lua, 3).and_then([&](V value) -> result<int> {
                    if constexpr (std::is_const_v<V>) {
                        return unexpected("lua: attempt to set const field");
                    } else {
                        (self.*field.field_ptr_) = std::move(value);
                    }

                    return 0;
                });
            });
        }(std::get<FieldIndex>(registration::fields))
                   .or_else([&](const std::string& error) -> result<int> {
                       lua_pushstring(lua, error.data());
                       lua_error(lua);

                       return unexpected("lua: unreachable post lua_error code reached");
                   })
                   .value();
    }

    template <std::size_t I, typename M>
    static void add_method(lua_State* lua, bound_method<M>& method)
    {
        lua_pushstring(lua, method.name_.data());
        lua_pushcfunction(lua, method_call<I>);
        lua_settable(lua, -3);
    }

    static int metatable__index_handler(lua_State* lua)
    {
        // stack: 1. this 2. key
        return from_lua<std::string>(lua, 2).and_then([&](std::string value) -> result<int> {
                                                auto pos = index_handlers.find(value);
                                                if (pos != index_handlers.end()) {
                                                    return pos->second(lua);
                                                } else {
                                                    return unexpected(std::format("no field or method '{}'", value));
                                                }
                                            })
            .or_else([&](const std::string& error) -> result<int> {
                lua_pushstring(lua, error.data());
                lua_error(lua);

                return unexpected("lua: unreachable post lua_error code reached");
            })
            .value();
    }

    static int metatable__newindex_handler(lua_State* lua)
    {
        // stack: 1. this 2. key 3. value
        return from_lua<std::string>(lua, 2).and_then([&](std::string value) -> result<int> {
                                                auto pos = newindex_handlers.find(value);
                                                if (pos != newindex_handlers.end()) {
                                                    return pos->second(lua);
                                                } else {
                                                    return unexpected(std::format("no field or method '{}'", value));
                                                }
                                            })
            .or_else([&](const std::string& error) -> result<int> {
                lua_pushstring(lua, error.data());
                lua_error(lua);

                return unexpected("lua: unreachable post lua_error code reached");
            })
            .value();
    }

    static void do_registration(lua_State* lua)
    {
        luaL_newmetatable(lua, registration::name.data());

        lua_pushstring(lua, "__gc");
        lua_pushcfunction(lua, &destructor);
        lua_settable(lua, -3);

        lua_pushstring(lua, "__index");
        lua_pushcfunction(lua, metatable__index_handler);
        lua_settable(lua, -3);

        lua_pushstring(lua, "__newindex");
        lua_pushcfunction(lua, metatable__newindex_handler);
        lua_settable(lua, -3);

        // pop the new metatable off the stack
        lua_pop(lua, 1);

        // add global same-name function for constructor
        lua_pushstring(lua, registration::name.data()); // env__index, "name"
        lua_pushcfunction(lua, constructor); // env__index, "name", constructor
        lua_settable(lua, -3); // env__index
    }

    static void do_deregistration() { }

    using methods_tuple = decltype(registration::methods);
    static constexpr std::size_t num_methods = std::tuple_size_v<methods_tuple>;

    using fields_tuple = decltype(registration::fields);
    static constexpr std::size_t num_fields = std::tuple_size_v<fields_tuple>;

    using index_handler = int (*)(lua_State*);
    static inline std::map<std::string, index_handler> index_handlers = []() {
        std::map<std::string, index_handler> result;
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((result[std::get<Is>(registration::methods).name_] = method_index_handler<Is>), ...);
        }(std::make_index_sequence<num_methods> {});
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((result[std::get<Is>(registration::fields).name_] = field_index_handler<Is>), ...);
        }(std::make_index_sequence<num_fields> {});
        return result;
    }();
    static inline std::map<std::string, index_handler> newindex_handlers = []() {
        std::map<std::string, index_handler> result;
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((result[std::get<Is>(registration::fields).name_] = field_newindex_handler<Is>), ...);
        }(std::make_index_sequence<num_fields> {});
        return result;
    }();
};
}
