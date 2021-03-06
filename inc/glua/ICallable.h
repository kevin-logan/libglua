#pragma once

#include <memory>
#include <tuple>
#include <type_traits>

namespace kdk {
class ICallable {
public:
    virtual auto Call() const -> void = 0;
    virtual auto HasReturn() const -> bool = 0;

    virtual auto GetImplementationData() const -> void* = 0;

    virtual ~ICallable() = default;
    ICallable() = default;
    ICallable(const ICallable&) = default;
    ICallable(ICallable&&) noexcept = default;

    auto operator=(const ICallable&) -> ICallable& = default;
    auto operator=(ICallable&&) noexcept -> ICallable& = default;
};

class Callable : public ICallable {
public:
    explicit Callable(std::unique_ptr<ICallable> callable);
    Callable(const Callable&) = delete;
    Callable(Callable&&) noexcept = default;

    auto operator=(const Callable&) -> Callable& = delete;
    auto operator=(Callable&&) noexcept -> Callable& = default;

    auto Call() const -> void override;

    auto HasReturn() const -> bool override;

    auto GetImplementationData() const -> void* override;

    auto AcquireCallable() && -> std::unique_ptr<ICallable>;

    ~Callable() override = default;

private:
    std::unique_ptr<ICallable> m_callable;
};

template <typename Functor>
class FunctorCallable : public ICallable {
public:
    explicit FunctorCallable(Functor functor)
        : m_functor(std::move(functor))
    {
    }
    FunctorCallable(const FunctorCallable&) = default;
    FunctorCallable(FunctorCallable&&) noexcept = default;

    auto operator=(const FunctorCallable&) -> FunctorCallable& = default;
    auto operator=(FunctorCallable&&) noexcept -> FunctorCallable& = default;

    auto Call() const -> void override { m_functor(); }

    auto HasReturn() const -> bool override { return false; }

    auto GetImplementationData() const -> void* override
    {
        // default implementation has no data
        return nullptr;
    }

    ~FunctorCallable() override = default;

private:
    Functor m_functor;
};

template <typename ArgumentStack, typename Functor, typename... Params>
class DeferredArgumentCallable : public ICallable {
public:
    using ReturnType = typename std::invoke_result<Functor, Params...>::type;
    explicit DeferredArgumentCallable(Functor functor)
        : m_functor(std::move(functor))
    {
    }
    DeferredArgumentCallable(const DeferredArgumentCallable&) = default;
    DeferredArgumentCallable(DeferredArgumentCallable&&) noexcept = default;

    auto operator=(const DeferredArgumentCallable&) -> DeferredArgumentCallable& = default;
    auto operator=(DeferredArgumentCallable&&) noexcept -> DeferredArgumentCallable& = default;

    template <size_t... Is>
    auto GetArgumentTuple(std::index_sequence<Is...> /*unused*/) const -> std::tuple<Params...>
    {
        return std::tuple<Params...> { ArgumentStack::template deferred_argument_get<Params>(this, Is)... };
    }

    auto Call() const -> void override
    {
        if constexpr (std::tuple_size<std::tuple<Params...>>::value > 0) {
            auto arg_tuple = GetArgumentTuple(std::index_sequence_for<Params...> {});

            if constexpr (std::is_same<ReturnType, void>::value) {
                std::apply(m_functor, std::move(arg_tuple));
            } else {
                if constexpr (std::is_reference<ReturnType>::value) {
                    ArgumentStack::deferred_argument_push(this, std::ref(std::apply(m_functor, std::move(arg_tuple))));
                } else {
                    ArgumentStack::deferred_argument_push(this, std::apply(m_functor, std::move(arg_tuple)));
                }
            }
        } else {
            if constexpr (std::is_same<ReturnType, void>::value) {
                m_functor();
            } else {
                if constexpr (std::is_reference<ReturnType>::value) {
                    ArgumentStack::deferred_argument_push(this, std::ref(m_functor()));
                } else {
                    ArgumentStack::deferred_argument_push(this, m_functor());
                }
            }
        }
    }

    auto HasReturn() const -> bool override { return !std::is_same<ReturnType, void>::value; }

    auto GetImplementationData() const -> void* override
    {
        // default implementation has no data
        return nullptr;
    }

    ~DeferredArgumentCallable() override = default;

private:
    Functor m_functor;
};

template <typename Functor>
auto create_functor_callable(Functor&& functor) -> Callable
{
    return Callable { std::make_unique<FunctorCallable<Functor>>(std::forward<Functor>(functor)) };
}

template <typename ArgumentStack, typename Functor, typename... Params>
auto create_deferred_argument_callable(Functor&& f) -> Callable
{
    return Callable {
        std::make_unique<DeferredArgumentCallable<ArgumentStack, Functor, Params...>>(std::forward<Functor>(f))
    };
}

template <typename ArgumentStack, typename ReturnType, typename... Params>
auto create_deferred_argument_callable(ReturnType (*callable)(Params...)) -> Callable
{
    return Callable { std::make_unique<DeferredArgumentCallable<ArgumentStack, decltype(callable), Params...>>(callable) };
}

template <typename ArgumentStack, typename ClassType, typename ReturnType, typename... Params>
auto create_deferred_argument_callable(ReturnType (ClassType::*callable)(Params...) const) -> Callable
{
    auto method_call_lambda = [callable](const ClassType& object, Params... params) {
        return (object.*callable)(std::forward<Params>(params)...);
    };

    return Callable {
        std::make_unique<DeferredArgumentCallable<ArgumentStack, decltype(method_call_lambda), const ClassType&, Params...>>(
            std::move(method_call_lambda))
    };
}

template <typename ArgumentStack, typename ClassType, typename ReturnType, typename... Params>
auto create_deferred_argument_callable(ReturnType (ClassType::*callable)(Params...)) -> Callable
{
    auto method_call_lambda = [callable](ClassType& object, Params... params) {
        return (object.*callable)(std::forward<Params>(params)...);
    };

    return Callable {
        std::make_unique<DeferredArgumentCallable<ArgumentStack, decltype(method_call_lambda), ClassType&, Params...>>(
            std::move(method_call_lambda))
    };
}

template <typename... Params>
struct OverloadResolver {
    template <typename ArgumentStack, typename Functor>
    static auto CreateDeferredCallable(Functor&& f) -> Callable
    {
        return create_deferred_argument_callable<ArgumentStack, Functor, Params...>(std::forward<Functor>(f));
    }

    template <typename ArgumentStack, typename ReturnType>
    static auto CreateDeferredCallable(ReturnType (*function)(Params...)) -> Callable
    {
        return create_deferred_argument_callable<ArgumentStack>(function);
    }

    template <typename ArgumentStack, typename ClassType, typename ReturnType>
    static auto CreateDeferredCallable(ReturnType (ClassType::*method)(Params...) const) -> Callable
    {
        return create_deferred_argument_callable<ArgumentStack>(method);
    }

    template <typename ArgumentStack, typename ClassType, typename ReturnType>
    static auto CreateDeferredCallable(ReturnType (ClassType::*method)(Params...)) -> Callable
    {
        return create_deferred_argument_callable<ArgumentStack>(method);
    }
};

} // namespace kdk
