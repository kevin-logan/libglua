#pragma once

#include "glua/glua.hpp"

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include <format>
#include <map>
#include <utility>

#include "lua_impl/converter_declarations.hpp"

// depends on conversions
#include "lua_impl/class_registration_impl.hpp"

// any implementations depend on converter declarations and class_registration_impl
#include "lua_impl/any.hpp"

// depends on class registration
#include "lua_impl/converter_definitions.hpp"

// depends on all conversions
#include "lua_impl/generic_functor.hpp"

namespace glua::lua {

class backend {
public:
    static result<std::unique_ptr<backend>> create(bool start_sandboxed = true)
    {
        return std::unique_ptr<backend>(new backend { start_sandboxed });
    }

    template <typename ReturnType>
    result<ReturnType> execute_script(const std::string& code)
    {
        auto top = lua_gettop(lua_);
        auto retval = [&]() -> result<ReturnType> {
            auto result_code = luaL_loadbuffer(lua_, code.data(),
                code.size(), "glua-lua");

            if (result_code == 0) {
                push_env();
                lua_setfenv(lua_, -2);
                result_code = lua_pcall(lua_, 0, std::same_as<ReturnType, void> ? 0 : 1, 0);

                if (result_code != 0) {
                    return unexpected(std::format("Failed to call script: {}", lua_tostring(lua_, -1)));
                }

                if constexpr (std::same_as<ReturnType, void>) {
                    return {};
                } else {
                    return from_lua<ReturnType>(lua_, -1);
                }
            } else {
                return unexpected(std::format("Failed to load script: {}", lua_tostring(lua_, -1)));
            }
        }();
        lua_settop(lua_, top);

        return retval;
    }

    template <typename ReturnType, typename... ArgTypes>
    result<void> register_functor(const std::string& name, generic_functor<ReturnType, ArgTypes...>& functor)
    {
        push_env__index();
        lua_pushstring(lua_, name.data());

        lua_pushlightuserdata(lua_, &functor);
        lua_pushcclosure(lua_, callback_for(functor), 1);

        lua_settable(lua_, -3); // stack was: env__index, name, closure

        lua_pop(lua_, 1); // pop env__index back off the stack

        return {};
    }

    template <typename T>
    result<T> get_global(const std::string& name)
    {
        push_env();
        lua_pushstring(lua_, name.data());

        lua_gettable(lua_, -2);

        auto result = from_lua<T>(lua_, -2);
        lua_pop(lua_, 2); // pop value and env off stack

        return result;
    }

    template <typename T>
    result<void> set_global(const std::string& name, T value)
    {
        push_env();
        lua_pushstring(lua_, name.data());

        return push_to_lua(lua_, std::move(value))
            .transform([&]() {
                lua_settable(lua_, -3); // stack was: env, name, value
                lua_pop(lua_, 1); // only env__index left on stack
            })
            .transform_error([&](auto error) {
                lua_pop(lua_, 2); // pop name and env
                return error;
            });

        return {};
    }

    template <typename ReturnType, typename... Args>
    result<ReturnType> call_function(const std::string& name, Args&&... args)
    {
        auto starting_top = lua_gettop(lua_);

        // push function then args in order
        push_env(); // env
        lua_pushstring(lua_, name.data()); // env, "name"

        lua_gettable(lua_, -2); // env, env["name"]
        lua_pushvalue(lua_, -2); // env, env["name"], env
        lua_setfenv(lua_, -2); // env, env["name"]

        auto retval = many_push_to_lua(lua_, std::forward<Args>(args)...).and_then([&]() -> result<ReturnType> {
            // stack now: env, env["name"], args...
            auto call_result = lua_pcall(lua_, sizeof...(Args), std::same_as<ReturnType, void> ? 0 : 1, 0);

            if (call_result != 0) {
                return unexpected(std::format("lua call failed: {}", lua_tostring(lua_, -1)));
            }

            if constexpr (std::same_as<ReturnType, void>) {
                return {};
            } else {
                return from_lua<ReturnType>(lua_, -1);
            }
        });

        // pop off anything leftover, as many_push_to_lua could have failed mid-pushing
        // so we can't determine the number of items to pop at compile time
        lua_settop(lua_, starting_top);

        return retval;
    }

    template <registered_class T>
    void register_class()
    {
        push_env__index(); // push env__index so registration can add globals if needed
        class_registration_impl<T>::do_registration(lua_);
        lua_pop(lua_, 1); // remove env__index
    }

    void push_env()
    {
        lua_getglobal(lua_, env_name);
    }

    void push_env__index()
    {
        lua_getglobal(lua_, env__index_name);
    }

    ~backend()
    {
        lua_close(lua_);
    }

private:
    backend(bool start_sandboxed)
        : lua_(luaL_newstate())
    {
        luaL_openlibs(lua_);

        build_sandbox(start_sandboxed);

        // let's create the env table
        lua_newtable(lua_); // env
        lua_pushvalue(lua_, -1); // env, env
        lua_pushliteral(lua_, "__index"); // env, env, __index
        push_env__index(); // env, env, __index, env__index
        lua_settable(lua_, -3); // env, env
        lua_setmetatable(lua_, -2); // env

        lua_setglobal(lua_, env_name); // empty stack
    }

    void build_sandbox(bool start_sandboxed)
    {
        if (start_sandboxed) {
            // lookup the global for each of these and push them onto env
            std::vector<std::string> sandbox_environment {
                "assert", "error", "ipairs", "next", "pairs",
                "pcall", "print", "select", "tonumber", "tostring",
                "type", "unpack", "_VERSION", "xpcall", "isfunction"
            };
            std::map<std::string, std::vector<std::string>> sandbox_sub_environments = {
                { "coroutine", std::vector<std::string> { "create", "resume", "running", "status", "wrap", "yield" } },
                { "io", std::vector<std::string> { "read", "write", "flush", "type" } },
                { "string", std::vector<std::string> { "byte", "char", "dump", "find", "format", "gmatch", "gsub", "len", "lower", "match", "rep", "reverse", "sub", "upper" } },
                { "table", std::vector<std::string> { "insert", "maxn", "remove", "sort", "concat" } },
                { "math", std::vector<std::string> { "abs", "acos", "asin", "atan", "atan2", "ceil", "cos", "cosh", "deg", "exp", "floor", "fmod", "frexp", "huge", "ldexp", "log", "log10", "max", "min", "modf", "pi", "pow", "rad", "random", "sin", "sinh", "sqrt", "tan", "tanh" } },
                { "os", std::vector<std::string> { "clock", "difftime", "time" } }
            };

            lua_newtable(lua_); // env__index;
            for (const auto& f : sandbox_environment) {
                lua_pushstring(lua_, f.data());
                lua_getglobal(lua_, f.data());
                lua_settable(lua_, -3); // set env__index["name"] = globals["name"]
            }

            // top of stack is still env__index
            for (const auto& [sub_environment_name, items] : sandbox_sub_environments) {
                lua_pushstring(lua_, sub_environment_name.data()); // env__index, "subenv"
                lua_newtable(lua_); // env__index, "subenv", subenv
                lua_getglobal(lua_, sub_environment_name.data()); // env__index, "subenv", subenv, globals["subenv"]
                for (const auto& f : items) {
                    // need to get f from coroutine table, push onto env_index["subenv"];
                    lua_pushstring(lua_, f.data()); // env__index, "subenv", subenv, globals["subenv"], "name"
                    lua_pushvalue(lua_, -1); // env__index, "subenv", subenv, globals["subenv"], "name", "name"
                    lua_gettable(lua_, -3); // env__index, "subenv", subenv, globals["subenv"], "name", globals["subenv"]["name"]
                    lua_settable(lua_, -4); // env__index, "subenv", subenv, globals["subenv"]
                }
                lua_pop(lua_, 1); // env__index, "subenv", subenv
                lua_settable(lua_, -3); // env__index
            }

            lua_setglobal(lua_, env__index_name); // stack now back to start
        } else {
            // when not using a sandbox:
            //   env.__index -> _G
            // where _G is the default lua globals table
            lua_getglobal(lua_, "_G");
            lua_setglobal(lua_, env__index_name);
        }
    }

    static constexpr char env_name[] = "__glua_env";
    static constexpr char env__index_name[] = "__glua_env__index";

    lua_State* lua_;
};

} // namespace glua::lua
