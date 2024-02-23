#pragma once

#include <memory>

namespace glua {
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
}
