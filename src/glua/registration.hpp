#pragma once

#include <string>
#include <string_view>
#include <type_traits>

#include "generic_functor.hpp"

namespace glua {

template <typename T>
struct bound_method {
    constexpr bound_method(std::string name, T generic_functor_ptr)
        : name_(std::move(name))
        , generic_functor_ptr_(std::move(generic_functor_ptr))
    {
    }
    std::string name_;
    T generic_functor_ptr_;
};

template <typename T>
constexpr auto bind_method(std::string name, T method)
{
    return bound_method { std::move(name), create_generic_functor(method) };
}

template <typename T>
struct bound_field {
    bound_field(std::string name, T field_ptr)
        : name_(std::move(name))
        , field_ptr_(std::move(field_ptr))
    {
    }
    std::string name_;
    T field_ptr_;
};

template <typename T>
constexpr auto bind_field(std::string name, T field)
{
    return bound_field { std::move(name), field };
}

template <typename T>
struct class_registration {
};

template <typename T>
concept registered_class = requires(class_registration<T> b) {
    {
        b.name
    } -> decays_to<std::string>;
    {
        b.constructor
    };
    {
        b.methods
    };
    {
        b.fields
    };
};

template <typename T>
concept decays_to_registered_class = registered_class<std::decay_t<T>>;

constexpr auto get_name_from_field_access(std::string_view pointer_name)
{
    return pointer_name.substr(pointer_name.rfind("::") + 2);
}

template <typename T>
struct is_method : std::false_type {
};

template <typename Class, typename ReturnType, typename... ArgTypes>
struct is_method<ReturnType (Class::*)(ArgTypes...)> : std::true_type {
};

template <typename Class, typename ReturnType, typename... ArgTypes>
struct is_method<ReturnType (Class::*)(ArgTypes...) const> : std::true_type {
};

template <typename T>
constexpr auto bind(std::string_view name, T ptr)
{
    if constexpr (is_method<std::decay_t<T>>::value) {
        return glua::bind_method(std::string { name }, ptr);
    } else {
        return glua::bind_field(std::string { name }, ptr);
    }
}

template <typename... Args, typename ReturnType>
auto resolve_overload(ReturnType (*ptr)(Args...)) -> ReturnType (*)(Args...)
{
    return ptr;
}
template <typename... Args, typename ReturnType, typename ClassType>
auto resolve_overload(ReturnType (ClassType::*ptr)(Args...)) -> ReturnType (ClassType::*)(Args...)
{
    return ptr;
}
template <typename... Args, typename ReturnType, typename ClassType>
auto resolve_overload(ReturnType (ClassType::*ptr)(Args...) const) -> ReturnType (ClassType::*)(Args...) const
{
    return ptr;
}

#define GLUABIND(field) glua::bind(glua::get_name_from_field_access(#field), &field)

// useful for overloaded methods, as glua cannot determine which overload should be bound
// and glua doesn't support binding overloaded methods given many languages do not support
// overloading themselves
// e.g.:
//  struct foo {
//      void do_something(bool);
//      void do_something(string);
//  };
// could be bound as:
//  GLUABIND_AS(&foo::do_something, void (foo::*)(bool))
// to bind the bool-parameter overload
#define GLUABIND_AS(field, as_type) glua::bind(glua::get_name_from_field_access(#field), static_cast<as_type>(&field))

#define GLUABIND_OVERLOAD(field, ...) \
    glua::bind(glua::get_name_from_field_access(#field), overloaded<__VA_ARGS__>::resolve(&field))
}
