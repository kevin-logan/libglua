#pragma once

// NOTE: Do not include this, include glua/backends/lua.hpp instead, the include order is carefully
// crafted to separate declarations and dependent definitions

namespace glua::lua {

template <typename ReturnType, typename... ArgTypes>
int call_generic_wrapped_functor(generic_functor<ReturnType, ArgTypes...>& f, lua_State* lua)
{
    int stack_size = lua_gettop(lua);
    if (std::cmp_less(stack_size, sizeof...(ArgTypes))) {
        auto error = std::format("incorrect number of arguments, stack_size {}, args {}", stack_size, sizeof...(ArgTypes));
        lua_pushstring(lua, error.data()); //"incorrect number of arguments");
        lua_error(lua); // throws, no return
    }

    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        // first arg is at index 1
        return many_from_lua<ArgTypes...>(lua, (1 + Is)...).and_then([&](auto args) -> result<int> {
            if constexpr (std::same_as<ReturnType, void>) {
                std::apply(
                    [&](auto&&... unwrapped_args) { f.call(std::forward<decltype(unwrapped_args)>(unwrapped_args)...); }, std::move(args));

                return 0;
            } else {
                return push_to_lua(lua, std::apply([&](auto&&... unwrapped_args) -> ReturnType { return f.call(std::forward<decltype(unwrapped_args)>(unwrapped_args)...); }, std::move(args)))
                    .transform([]() { return 1; }); // 1 for 1 return value
            }
        });
    }(std::index_sequence_for<ArgTypes...> {})
               .or_else([&](const auto& error) -> result<int> {
                   lua_pushstring(lua, error.data());
                   lua_error(lua);

                   return unexpected("lua: unreachable post lua_error code reached");
               })
               .value();
}

template <typename ReturnType, typename... ArgTypes>
int generic_wrapped_functor(lua_State* lua)
{
    // functor pointer stored as upvalue
    auto* f = static_cast<generic_functor<ReturnType, ArgTypes...>*>(lua_touserdata(lua, lua_upvalueindex(1)));
    return call_generic_wrapped_functor(*f, lua);
}

template <typename ReturnType, typename... ArgTypes>
auto callback_for(const generic_functor<ReturnType, ArgTypes...>&)
{
    return &generic_wrapped_functor<ReturnType, ArgTypes...>;
}
}
