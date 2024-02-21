#pragma once

#include <expected>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

namespace glua {

#if __cplusplus > 202002L && __cpp_concepts >= 202002L
template <typename T>
using result = std::expected<T, std::string>;
using unexpected = std::unexpected<std::string>;
#else

// what follows is a shoddy expected implementation for clang until std::expected is supported
// this implementation does _far_ less checking to ensure sanity between types and will produce
// errors when misused that are much more difficult to understand
struct unexpected {
    std::string error_;
};

template <typename F, typename A>
struct invoke_result : std::invoke_result<F, A> { };

template <typename F>
struct invoke_result<F, void> : std::invoke_result<F> { };

template <typename T>
class result {
public:
    using ValueType = std::conditional_t<std::same_as<T, void>, std::monostate, T>;

    template <typename U>
    friend class result;

    template <typename... FakeArgs>
        requires(std::same_as<T, void> && sizeof...(FakeArgs) == 0)
    result(FakeArgs&&...)
        : value_(std::in_place_index_t<0> {}, std::monostate {})
    {
    }

    template <typename U = T>
    explicit(!std::is_convertible_v<U, T>) result(U&& value)
        : value_(std::in_place_index_t<0> {}, std::forward<U>(value))
    {
    }
    result(unexpected error)
        : value_(std::in_place_index_t<1> {}, std::move(error.error_))
    {
    }

    result(const result&) = default;
    result(result&&) = default;

    template <typename U>
        requires(!std::same_as<T, void> && !std::same_as<U, void>)
    result(const result<U>& copy_convert)
        : value_([&]() {
            if (copy_convert.value_.index() == 0) {
                return std::variant<ValueType, std::string> { std::in_place_index_t<0> {}, std::get<0>(copy_convert.value_) };
            } else {
                return std::variant<ValueType, std::string> { std::in_place_index_t<1> {}, std::get<1>(copy_convert.value_) };
            }
        }())
    {
    }

    template <typename U>
        requires(!std::same_as<T, void> && !std::same_as<U, void>)
    result(result<U>&& move_convert)
        : value_([&]() {
            if (move_convert.value_.index() == 0) {
                return std::variant<ValueType, std::string> { std::in_place_index_t<0> {}, std::get<0>(std::move(move_convert.value_)) };
            } else {
                return std::variant<ValueType, std::string> { std::in_place_index_t<1> {}, std::get<1>(std::move(move_convert.value_)) };
            }
        }())
    {
    }

    constexpr bool has_value() const noexcept { return value_.index() == 0; }

    constexpr const ValueType& value() const& noexcept { return std::get<0>(value_); }
    constexpr ValueType& value() & noexcept { return std::get<0>(value_); }
    constexpr const ValueType&& value() const&& noexcept { return std::get<0>(std::move(value_)); }
    constexpr ValueType&& value() && noexcept { return std::get<0>(std::move(value_)); }

    constexpr const std::string& error() const& noexcept { return std::get<1>(value_); }
    constexpr std::string& error() & noexcept { return std::get<1>(value_); }
    constexpr const std::string&& error() const&& noexcept { return std::get<1>(std::move(value_)); }
    constexpr std::string&& error() && noexcept { return std::get<1>(std::move(value_)); }

    template <typename U>
    ValueType value_or(U&& default_value) const&
    {
        if (value_.index() == 0) {
            return std::get<0>(value_);
        } else {
            return std::forward<U>(default_value);
        }
    }

    template <typename F>
    auto and_then(F&& f) const&
    {
        // F must return another result, so we either forward our error to that result type, or return the value
        // of that functor over our value
        using transformed_result = invoke_result<F&&, std::add_lvalue_reference_t<std::add_const_t<T>>>::type;
        if (value_.index() == 0)
            if constexpr (std::same_as<T, void>)
                return std::forward<F>(f)();
            else
                return std::forward<F>(f)(std::get<0>(value_));
        else
            return transformed_result { unexpected { std::get<1>(value_) } };
    }

    template <typename F>
    auto and_then(F&& f) &
    {
        // F must return another result, so we either forward our error to that result type, or return the value
        // of that functor over our value
        using transformed_result = invoke_result<F&&, std::add_lvalue_reference_t<T>>::type;
        if (value_.index() == 0)
            if constexpr (std::same_as<T, void>)
                return std::forward<F>(f)();
            else
                return std::forward<F>(f)(std::get<0>(value_));
        else
            return transformed_result { unexpected { std::get<1>(value_) } };
    }

    template <typename F>
    auto and_then(F&& f) const&&
    {
        // F must return another result, so we either forward our error to that result type, or return the value
        // of that functor over our value
        using transformed_result = invoke_result<F&&, std::add_rvalue_reference_t<std::add_const_t<T>>>::type;
        if (value_.index() == 0)
            if constexpr (std::same_as<T, void>)
                return std::forward<F>(f)();
            else
                return std::forward<F>(f)(std::get<0>(std::move(value_)));
        else
            return transformed_result { unexpected { std::get<1>(std::move(value_)) } };
    }

    template <typename F>
    auto and_then(F&& f) &&
    {
        // F must return another result, so we either forward our error to that result type, or return the value
        // of that functor over our value
        using transformed_result = invoke_result<F&&, std::add_rvalue_reference_t<T>>::type;
        if (value_.index() == 0)
            if constexpr (std::same_as<T, void>)
                return std::forward<F>(f)();
            else
                return std::forward<F>(f)(std::get<0>(std::move(value_)));
        else
            return transformed_result { unexpected { std::get<1>(std::move(value_)) } };
    }

    template <typename F>
    auto or_else(F&& f) const&
    {
        if (value_.index() == 0)
            return *this;
        else
            return std::forward<F>(f)(std::get<1>(value_));
    }

    template <typename F>
    auto or_else(F&& f) &
    {
        if (value_.index() == 0)
            return *this;
        else
            return std::forward<F>(f)(std::get<1>(value_));
    }

    template <typename F>
    auto or_else(F&& f) const&&
    {
        if (value_.index() == 0)
            return std::move(*this);
        else
            return std::forward<F>(f)(std::get<1>(std::move(value_)));
    }

    template <typename F>
    auto or_else(F&& f) &&
    {
        if (value_.index() == 0)
            return std::move(*this);
        else
            return std::forward<F>(f)(std::get<1>(std::move(value_)));
    }

    template <typename F>
    auto transform(F&& f) const&
    {
        using transformed_result = invoke_result<F&&, std::add_lvalue_reference_t<std::add_const_t<T>>>::type;
        if (value_.index() == 0)
            if constexpr (std::same_as<T, void>) {
                if constexpr (std::same_as<transformed_result, void>)
                    return (std::forward<F>(f)(), result<transformed_result> {});
                else
                    return result<transformed_result> { std::forward<F>(f)() };
            } else {
                if constexpr (std::same_as<transformed_result, void>)
                    return (std::forward<F>(f)(std::get<0>(value_)), result<transformed_result> {});
                else
                    return result<transformed_result> { std::forward<F>(f)(std::get<0>(value_)) };
            }
        else
            return result<transformed_result> { unexpected { std::get<1>(value_) } };
    }

    template <typename F>
    auto transform(F&& f) &
    {
        using transformed_result = invoke_result<F&&, std::add_lvalue_reference_t<T>>::type;
        if (value_.index() == 0)
            if constexpr (std::same_as<T, void>) {
                if constexpr (std::same_as<transformed_result, void>)
                    return (std::forward<F>(f)(), result<transformed_result> {});
                else
                    return result<transformed_result> { std::forward<F>(f)() };
            } else {
                if constexpr (std::same_as<transformed_result, void>)
                    return (std::forward<F>(f)(std::get<0>(value_)), result<transformed_result> {});
                else
                    return result<transformed_result> { std::forward<F>(f)(std::get<0>(value_)) };
            }
        else
            return result<transformed_result> { unexpected { std::get<1>(value_) } };
    }

    template <typename F>
    auto transform(F&& f) const&&
    {
        using transformed_result = invoke_result<F&&, std::add_rvalue_reference_t<std::add_const_t<T>>>::type;
        if (value_.index() == 0)
            if constexpr (std::same_as<T, void>) {
                if constexpr (std::same_as<transformed_result, void>)
                    return (std::forward<F>(f)(), result<transformed_result> {});
                else
                    return result<transformed_result> { std::forward<F>(f)() };
            } else {
                if constexpr (std::same_as<transformed_result, void>)
                    return (std::forward<F>(f)(std::get<0>(std::move(value_))), result<transformed_result> {});
                else
                    return result<transformed_result> { std::forward<F>(f)(std::get<0>(std::move(value_))) };
            }
        else
            return result<transformed_result> { unexpected { std::get<1>(std::move(value_)) } };
    }

    template <typename F>
    auto transform(F&& f) &&
    {
        using transformed_result = invoke_result<F&&, std::add_rvalue_reference_t<T>>::type;
        if (value_.index() == 0)
            if constexpr (std::same_as<T, void>) {
                if constexpr (std::same_as<transformed_result, void>)
                    return (std::forward<F>(f)(), result<transformed_result> {});
                else
                    return result<transformed_result> { std::forward<F>(f)() };
            } else {
                if constexpr (std::same_as<transformed_result, void>)
                    return (std::forward<F>(f)(std::get<0>(std::move(value_))), result<transformed_result> {});
                else
                    return result<transformed_result> { std::forward<F>(f)(std::get<0>(std::move(value_))) };
            }
        else
            return result<transformed_result> { unexpected { std::get<1>(std::move(value_)) } };
    }

    template <typename F>
    auto transform_error(F&& f) const&
    {
        if (value_.index() == 0)
            return *this;
        else
            return result<T> { unexpected { std::forward<F>(f)(std::get<1>(value_)) } };
    }

    template <typename F>
    auto transform_error(F&& f) &
    {
        if (value_.index() == 0)
            return *this;
        else
            return result<T> { unexpected { std::forward<F>(f)(std::get<1>(value_)) } };
    }

    template <typename F>
    auto transform_error(F&& f) const&&
    {
        if (value_.index() == 0)
            return std::move(*this);
        else
            return result<T> { unexpected { std::forward<F>(f)(std::get<1>(std::move(value_))) } };
    }

    template <typename F>
    auto transform_error(F&& f) &&
    {
        if (value_.index() == 0)
            return std::move(*this);
        else
            return result<T> { unexpected { std::forward<F>(f)(std::get<1>(std::move(value_))) } };
    }

private:
    std::variant<ValueType, std::string> value_;
};

#endif

template <std::size_t I, typename... Ts>
result<void> check_many_results(std::tuple<result<Ts>...>& t)
{
    if constexpr (I < sizeof...(Ts)) {
        if (std::get<I>(t).has_value()) {
            return check_many_results<I + 1>(t);
        } else {
            return unexpected(std::get<I>(t).error());
        }
    } else {
        // we got to the end, it's a success
        return {};
    }
}

template <typename... Ts>
result<std::tuple<Ts...>> many_results_to_one(std::tuple<result<Ts>...> input)
{
    return check_many_results<0>(input).transform([&]() {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return std::make_tuple(std::get<Is>(std::move(input)).value()...);
        }(std::index_sequence_for<Ts...> {});
    });
}

struct functor {
    virtual ~functor() = default;
};

template <typename ReturnType, typename... ArgTypes>
struct generic_functor : public functor {
    virtual ReturnType call(ArgTypes...) = 0;
    ~generic_functor() override = default;

    static constexpr std::size_t num_args { sizeof...(ArgTypes) };
};

template <typename Functor, typename ReturnType, typename... ArgTypes>
struct generic_functor_impl : public generic_functor<ReturnType, ArgTypes...> {
    constexpr generic_functor_impl(Functor f)
        : f_(std::move(f))
    {
    }

    ~generic_functor_impl() override = default;

    ReturnType call(ArgTypes... args) override { return f_(static_cast<ArgTypes&&>(args)...); }

    Functor f_;
};

template <typename ReturnType, typename... ArgTypes>
constexpr std::unique_ptr<generic_functor<ReturnType, ArgTypes...>> create_generic_functor(ReturnType (*fp)(ArgTypes...))
{
    return std::make_unique<generic_functor_impl<ReturnType (*)(ArgTypes...), ReturnType, ArgTypes...>>(fp);
}

template <typename Functor, typename ReturnType, typename... ArgTypes>
constexpr std::unique_ptr<generic_functor<ReturnType, ArgTypes...>>
create_generic_functor(Functor f, ReturnType (Functor::*)(ArgTypes...))
{
    return std::make_unique<generic_functor_impl<Functor, ReturnType, ArgTypes...>>(std::move(f));
}

template <typename Functor, typename ReturnType, typename... ArgTypes>
constexpr std::unique_ptr<generic_functor<ReturnType, ArgTypes...>>
create_generic_functor(Functor f, ReturnType (Functor::*)(ArgTypes...) const)
{
    return std::make_unique<generic_functor_impl<Functor, ReturnType, ArgTypes...>>(std::move(f));
}

template <typename Functor>
constexpr auto create_generic_functor(Functor f)
{
    return create_generic_functor(std::move(f), &Functor::operator());
}

template <typename ReturnType, typename ClassType, typename... ArgTypes>
constexpr std::unique_ptr<generic_functor<ReturnType, ClassType&, ArgTypes...>>
create_generic_functor(ReturnType (ClassType::*fp)(ArgTypes...))
{
    return create_generic_functor(
        [fp](ClassType& c, ArgTypes... args) -> ReturnType { return (c.*fp)(static_cast<ArgTypes&&>(args)...); });
}

template <typename ReturnType, typename ClassType, typename... ArgTypes>
constexpr std::unique_ptr<generic_functor<ReturnType, const ClassType&, ArgTypes...>>
create_generic_functor(ReturnType (ClassType::*fp)(ArgTypes...) const)
{
    return create_generic_functor(
        [fp](const ClassType& c, ArgTypes... args) -> ReturnType { return (c.*fp)(static_cast<ArgTypes&&>(args)...); });
}

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

template <typename T, typename U>
concept decays_to = std::same_as<std::decay_t<T>, U>;

template <typename T>
concept decays_to_integral = std::integral<std::decay_t<T>>;

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

// struct overloaded
//{
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
//};

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

template <typename Backend>
class instance {
public:
    template <typename... CreateArgs>
    static result<instance> create(CreateArgs&&... create_args)
    {
        return Backend::create(std::forward<CreateArgs>(create_args)...).transform([&](auto backend_ptr) { return instance { std::move(backend_ptr) }; });
    }

    template <typename ReturnType>
    result<ReturnType> execute_script(const std::string& code)
    {
        if constexpr (std::same_as<ReturnType, void>)
            backend_ptr_->template execute_script<ReturnType>(code);
        else
            return backend_ptr_->template execute_script<ReturnType>(code);
    }

    template <typename F>
    result<void> register_functor(const std::string& name, F functor)
    {
        auto generic_functor_ptr = create_generic_functor(std::move(functor));
        auto result = register_functor(name, *generic_functor_ptr);
        registered_functors_.push_back(std::move(generic_functor_ptr));
        return result;
    }

    template <typename ReturnType, typename... ArgTypes>
    result<void> register_functor(const std::string& name, generic_functor<ReturnType, ArgTypes...>& functor)
    {
        return backend_ptr_->register_functor(name, functor);
    }

    template <typename T>
    result<T> get_global(const std::string& name)
    {
        return backend_ptr_->template get_global<T>(name);
    }

    template <typename T>
    result<void> set_global(const std::string& name, T value)
    {
        return backend_ptr_->set_global(name, std::move(value));
    }

    template <typename ReturnType, typename... Args>
    result<ReturnType> call_function(const std::string& name, Args&&... args)
    {
        return backend_ptr_->template call_function<ReturnType>(name, std::forward<Args>(args)...);
    }

    template <registered_class T>
    void register_class()
    {
        return backend_ptr_->template register_class<T>();
    }

private:
    instance(std::unique_ptr<Backend> backend_ptr)
        : backend_ptr_(std::move(backend_ptr))
    {
    }

    std::unique_ptr<Backend> backend_ptr_;
    std::vector<std::unique_ptr<functor>> registered_functors_;
};
} // namespace glua