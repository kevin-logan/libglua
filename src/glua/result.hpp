#pragma once

#include <expected>
#include <string>
#include <tuple>

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
    unexpected(std::string error)
        : error_(std::move(error))
    {
    }
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

}
