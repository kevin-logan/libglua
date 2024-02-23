#pragma once

// NOTE: Do not include this, include glua/backends/spidermonkey.hpp instead, the include order is carefully
// crafted to separate declarations and dependent definitions

namespace glua::spidermonkey {
template <typename ReturnType, typename... ArgTypes>
bool call_generic_wrapped_functor(generic_functor<ReturnType, ArgTypes...>& f, JSContext* cx, JS::CallArgs& args)
{
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return many_from_js<ArgTypes...>(cx, args.get(Is)...).and_then([&](auto actual_args) -> result<void> {
            if constexpr (std::same_as<ReturnType, void>) {
                std::apply(
                    [&](auto&&... unwrapped_args) -> ReturnType {
                        f.call(std::forward<decltype(unwrapped_args)>(unwrapped_args)...);
                    },
                    std::move(actual_args));

                return {};
            } else {
                return to_js(
                    cx,
                    std::apply(
                        [&](auto&&... unwrapped_args) -> ReturnType {
                            return f.call(std::forward<decltype(unwrapped_args)>(unwrapped_args)...);
                        },
                        std::move(actual_args)))
                    .transform([&](auto v) { args.rval().set(v); });
            }
        });
    }(std::index_sequence_for<ArgTypes...> {})
               .transform([]() { return true; })
               .or_else([&](const auto& error) -> result<bool> {
                   JS_ReportErrorASCII(cx, "%s", error.data());
                   return false;
               })
               .value();
}

template <typename ReturnType, typename ClassType, typename... ArgTypes>
bool call_generic_wrapped_method(generic_functor<ReturnType, ClassType, ArgTypes...>& f, JSContext* cx, JS::CallArgs& args)
{
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return many_from_js<ClassType, ArgTypes...>(cx, args.thisv(), args.get(Is)...).and_then([&](auto actual_args) -> result<void> {
            if constexpr (std::same_as<ReturnType, void>) {
                std::apply(
                    [&](auto&&... unwrapped_args) -> ReturnType {
                        f.call(std::forward<decltype(unwrapped_args)>(unwrapped_args)...);
                    },
                    std::move(actual_args));

                return {};
            } else {
                return to_js(
                    cx,
                    std::apply(
                        [&](auto&&... unwrapped_args) -> ReturnType {
                            return f.call(std::forward<decltype(unwrapped_args)>(unwrapped_args)...);
                        },
                        std::move(actual_args)))
                    .transform([&](auto v) { args.rval().set(v); });
            }
        });
    }(std::index_sequence_for<ArgTypes...> {})
               .transform([]() { return true; })
               .or_else([&](const auto& error) -> result<bool> {
                   JS_ReportErrorASCII(cx, "%s", error.data());
                   return false;
               })
               .value();
}

template <typename ReturnType, typename... ArgTypes>
bool generic_wrapped_functor(JSContext* cx, unsigned argc, JS::Value* vp)
{
    if (argc != sizeof...(ArgTypes)) {
        JS_ReportErrorASCII(cx, "Invalid number of arguments");
        return false;
    }

    auto args = JS::CallArgsFromVp(argc, vp);
    JSObject& func = args.callee(); // actual functor should be stored here

    const JS::Value& v = js::GetFunctionNativeReserved(&func, 0);
    auto* functor = reinterpret_cast<generic_functor<ReturnType, ArgTypes...>*>(v.toPrivate());

    return call_generic_wrapped_functor(*functor, cx, args);
}

template <typename ReturnType, typename... ArgTypes>
JSNative callback_for(const generic_functor<ReturnType, ArgTypes...>&)
{
    return &generic_wrapped_functor<ReturnType, ArgTypes...>;
}
}
