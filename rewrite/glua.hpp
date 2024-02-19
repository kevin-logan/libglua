#pragma once

#include <expected>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>

namespace glua {
template <typename T>
using result = std::expected<T, std::string>;

template <std::size_t I, typename... Ts>
result<void> check_many_results(std::tuple<result<Ts>...>& t)
{
    if constexpr (I < sizeof...(Ts)) {
        if (std::get<I>(t).has_value()) {
            return check_many_results<I + 1>(t);
        } else {
            return std::unexpected(std::get<I>(t).error());
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

template <typename ReturnType, typename... ArgTypes>
struct generic_functor {
    virtual ReturnType call(ArgTypes...) = 0;

    static constexpr std::size_t num_args { sizeof...(ArgTypes) };
};

template <typename Functor, typename ReturnType, typename... ArgTypes>
struct generic_functor_impl : public generic_functor<ReturnType, ArgTypes...> {
    constexpr generic_functor_impl(Functor f)
        : f_(std::move(f))
    {
    }

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
create_generic_functor(Functor f, ReturnType (Functor::*dummy)(ArgTypes...))
{
    return std::make_unique<generic_functor_impl<Functor, ReturnType, ArgTypes...>>(std::move(f));
}

template <typename Functor, typename ReturnType, typename... ArgTypes>
constexpr std::unique_ptr<generic_functor<ReturnType, ArgTypes...>>
create_generic_functor(Functor f, ReturnType (Functor::*dummy)(ArgTypes...) const)
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

template <typename Backend>
class instance {
public:
    static result<instance> create()
    {
        return Backend::create().transform([&](auto backend_ptr) { return instance { std::move(backend_ptr) }; });
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
    auto register_functor(const std::string& name, F functor)
    {
        auto generic_functor_ptr = create_generic_functor(std::move(functor));
        return register_functor(name, *generic_functor_ptr).transform([&]() { return std::move(generic_functor_ptr); });
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
};
} // namespace glua