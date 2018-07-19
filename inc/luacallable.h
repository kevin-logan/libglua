#pragma once

#include "exceptions.h"
#include "icallable.h"

#include <sstream>
#include <string>
#include <string_view>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#define REGISTER_TO_LUA(lua, functor) lua->RegisterCallable(#functor, lua->CreateLuaCallable(functor))

#define REGISTER_CLASS_TO_LUA(lua, ClassType, ...) lua->RegisterClassMultiString<ClassType>(#__VA_ARGS__, __VA_ARGS__)

namespace kdk::lua
{
class Lua : public std::enable_shared_from_this<Lua>
{
public:
    static auto Create() -> std::shared_ptr<Lua>;

    template<typename T>
    static auto get(const ICallable* callable, int32_t stack_index) -> T;

    template<typename T>
    static auto get(lua_State* lua, int32_t stack_index) -> T;

    template<typename T>
    static auto push(const ICallable* callable, T value) -> void;

    auto Push(bool value) -> void;
    auto Push(int8_t value) -> void;
    auto Push(int16_t value) -> void;
    auto Push(int32_t value) -> void;
    auto Push(int64_t value) -> void;
    auto Push(uint8_t value) -> void;
    auto Push(uint16_t value) -> void;
    auto Push(uint32_t value) -> void;
    auto Push(uint64_t value) -> void;
    auto Push(float value) -> void;
    auto Push(double value) -> void;
    auto Push(const char* value) -> void;
    auto Push(std::string_view value) -> void;
    auto Push(std::string value) -> void;

    template<typename T>
    auto Push(T value) -> void;

    template<typename T>
    auto Pop() -> T;

    template<typename Functor, typename... Params>
    auto CreateLuaCallable(Functor&& f) -> Callable;

    template<typename ReturnType, typename... Params>
    auto CreateLuaCallable(ReturnType (*callable)(Params...)) -> Callable;

    template<typename ClassType, typename ReturnType, typename... Params>
    auto CreateLuaCallable(ReturnType (ClassType::*callable)(Params...) const) -> Callable;

    template<typename ClassType, typename ReturnType, typename... Params>
    auto CreateLuaCallable(ReturnType (ClassType::*callable)(Params...)) -> Callable;

    auto RunScript(std::string_view script_data) -> void;
    auto RunFile(std::string_view file_name) -> void;

    auto CallScriptFunction(const std::string& function_name) -> void;

    template<typename... Params>
    auto CallScriptFunction(const std::string& function_name, Params&&... params) -> void;

    auto RegisterCallable(std::string_view name, Callable callable) -> void;

    template<typename ClassType, typename... Methods>
    auto RegisterClassMultiString(std::string_view method_names, Methods... methods) -> void;

    template<typename ClassType, typename... Methods>
    auto RegisterClass(std::vector<std::string_view> method_names, std::vector<std::unique_ptr<ICallable>> methods)
        -> void;

    auto GetLuaState() -> lua_State*;
    auto GetLuaState() const -> const lua_State*;

    auto GetNewOutput() -> std::string;

    template<typename T>
    auto GetMetatableName() -> std::optional<std::string>;

    template<typename T>
    auto SetMetatableName(std::string metatable_name) -> void;

    ~Lua();

private:
    Lua();

    lua_State* m_lua;

    std::unordered_map<std::string, std::unique_ptr<ICallable>>                                  m_registry;
    std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<ICallable>>> m_method_registry;
    std::unordered_map<std::type_index, std::string> m_class_to_metatable_name;

    std::stringstream m_output_stream;

    static std::unique_ptr<Lua> g_instance;
};

auto call_callable_from_lua(lua_State* state) -> int;

auto remove_all_whitespace(std::string_view input) -> std::string;
auto split(std::string_view input, std::string_view token) -> std::vector<std::string_view>;

template<typename Functor, typename... Params>
class LuaCallable : public DeferredArgumentCallable<Lua, Functor, Params...>
{
public:
    LuaCallable(std::shared_ptr<Lua> lua, Functor functor)
        : DeferredArgumentCallable<Lua, Functor, Params...>(std::move(functor)), m_lua(lua)
    {
    }
    LuaCallable(LuaCallable&&) = default;

    auto GetLua() const -> std::shared_ptr<Lua>;

    auto GetImplementationData() const -> void* override { return m_lua.get(); }

    ~LuaCallable() override = default;

private:
    std::shared_ptr<Lua> m_lua;
};

template<typename Functor, typename... Params>
auto LuaCallable<Functor, Params...>::GetLua() const -> std::shared_ptr<Lua>
{
    return m_lua;
}

template<typename T>
auto Lua::get(const ICallable* callable, int32_t stack_index) -> T
{
    auto* lua_ptr = static_cast<Lua*>(callable->GetImplementationData());
    if (lua_ptr)
    {
        return get<T>(lua_ptr->GetLuaState(), stack_index);
    }
    else
    {
        throw exceptions::LuaException("Tried to get Lua value for a non-Lua callable");
    }
}

template<typename T, typename = void>
struct ManagedTypeHandler
{
    static auto get(lua_State*, size_t) -> T { throw exceptions::LuaException("Tried to get value of unmanaged type"); }

    static auto push(lua_State*, T) -> void { throw exceptions::LuaException("Tried to push value of unmanaged type"); }
};

template<typename T>
struct ManagedTypeHandler<T*, std::void_t<decltype(std::declval<T*>()->shared_from_this())>>
{
    static auto get(lua_State* state, int32_t stack_index) -> T*
    {
        return ManagedTypeHandler<std::shared_ptr<T>>::get(state, stack_index).get();
    }

    static auto push(lua_State* state, T* value) -> void
    {
        ManagedTypeHandler<std::shared_ptr<T>>::push(state, value->shared_from_this());
    }
};

template<typename T>
struct ManagedTypeHandler<T&, std::void_t<decltype(std::declval<T&>().shared_from_this())>>
{
    static auto get(lua_State* state, int32_t stack_index) -> T&
    {
        return *ManagedTypeHandler<std::shared_ptr<T>>::get(state, stack_index);
    }

    static auto push(lua_State* state, T& value) -> void
    {
        ManagedTypeHandler<std::shared_ptr<T>>::push(state, value.shared_from_this());
    }
};

template<typename T>
struct ManagedTypeHandler<std::shared_ptr<T>>
{
    static auto get(lua_State* state, int32_t stack_index) -> std::shared_ptr<T>
    {
        lua_getglobal(state, "LuaClass");
        auto* lua_object = static_cast<Lua*>(lua_touserdata(state, -1));
        lua_pop(state, 1);

        auto metatable_name_opt = lua_object->GetMetatableName<T>();

        auto* shared_ptr_ptr = static_cast<std::shared_ptr<T>**>(
            luaL_checkudata(state, static_cast<int>(stack_index), metatable_name_opt.value().data()));

        if (shared_ptr_ptr != nullptr)
        {
            return **shared_ptr_ptr;
        }
        else
        {
            throw kdk::exceptions::LuaException("Tried to get type but invalid value at index!");
        }
    }

    static auto push(lua_State* state, std::shared_ptr<T> value) -> void
    {
        lua_getglobal(state, "LuaClass");
        auto* lua_object = static_cast<Lua*>(lua_touserdata(state, -1));
        lua_pop(state, 1);

        auto metatable_name_opt = lua_object->GetMetatableName<T>();

        if (metatable_name_opt)
        {
            auto shared_ptr_ptr_ptr =
                static_cast<std::shared_ptr<T>**>(lua_newuserdata(state, sizeof(std::shared_ptr<T>*)));

            *shared_ptr_ptr_ptr = new std::shared_ptr<T>{value};
            auto metatable_name = metatable_name_opt.value();

            // set the metatable for this object
            luaL_getmetatable(state, metatable_name.data());

            if (lua_isnil(state, -1))
            {
                lua_pop(state, 1);
                delete *shared_ptr_ptr_ptr;

                throw std::runtime_error(
                    "Pushing class type which has not been registered [null metatable]! " + metatable_name);
            }
            lua_setmetatable(state, -2);
        }
        else
        {
            throw std::runtime_error("Puhsing class type which has not been registered [no metatable name]!");
        }
    }
};

template<typename T>
auto Lua::get(lua_State* state, int32_t stack_index) -> T
{
    return ManagedTypeHandler<T>::get(state, stack_index);
}

template<typename T>
auto Lua::push(const ICallable* callable, T value) -> void
{
    auto* lua_ptr = static_cast<Lua*>(callable->GetImplementationData());
    if (lua_ptr)
    {
        lua_ptr->Push(std::move(value));
    }
    else
    {
        throw exceptions::LuaException("Tried to push Lua value for a non-Lua callable");
    }
}

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> bool;

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> int8_t;

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> int16_t;

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> int32_t;

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> int64_t;

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> uint8_t;

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> uint16_t;

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> uint32_t;

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> uint64_t;

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> float;

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> double;

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> std::string;

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> std::string_view;

template<typename T>
auto Lua::Push(T value) -> void
{
    return ManagedTypeHandler<T>::push(m_lua, std::move(value));
}

template<typename T>
auto Lua::Pop() -> T
{
    auto val = get<T>(m_lua, -1);
    lua_pop(m_lua, 1);

    return val;
}

template<typename Functor, typename... Params>
auto Lua::CreateLuaCallable(Functor&& f) -> Callable
{
    return Callable{std::make_unique<LuaCallable<Functor, Params...>>(shared_from_this(), std::forward<Functor>(f))};
}

template<typename ReturnType, typename... Params>
auto Lua::CreateLuaCallable(ReturnType (*callable)(Params...)) -> Callable
{
    return Callable{std::make_unique<LuaCallable<decltype(callable), Params...>>(shared_from_this(), callable)};
}

template<typename ClassType, typename ReturnType, typename... Params>
auto Lua::CreateLuaCallable(ReturnType (ClassType::*callable)(Params...) const) -> Callable
{
    auto method_call_lambda = [callable](const ClassType& object, Params... params) {
        return (object.*callable)(std::forward<Params>(params)...);
    };

    return Callable{std::make_unique<LuaCallable<decltype(method_call_lambda), const ClassType&, Params...>>(
        shared_from_this(), std::move(method_call_lambda))};
}

template<typename ClassType, typename ReturnType, typename... Params>
auto Lua::CreateLuaCallable(ReturnType (ClassType::*callable)(Params...)) -> Callable
{
    auto method_call_lambda = [callable](ClassType& object, Params... params) {
        return (object.*callable)(std::forward<Params>(params)...);
    };

    return Callable{std::make_unique<LuaCallable<decltype(method_call_lambda), ClassType&, Params...>>(
        shared_from_this(), std::move(method_call_lambda))};
}

template<typename... Params>
auto Lua::CallScriptFunction(const std::string& function_name, Params&&... params) -> void
{
    lua_getglobal(m_lua, function_name.data());

    if (!lua_isfunction(m_lua, -1))
    {
        throw exceptions::LuaException(
            "Attempted to call lua script function" + function_name + " which was not a function");
    }

    // push all params onto the lua stack
    ((Push(std::forward<Params>(params))), ...);

    if (lua_pcall(m_lua, sizeof...(Params), LUA_MULTRET, 0))
    {
        throw exceptions::LuaException("Failed to call lua script function" + function_name);
    }
}

template<typename Method, typename... Methods>
auto emplace_methods(Lua& lua, std::vector<std::unique_ptr<ICallable>>& v, Method m, Methods... methods) -> void
{
    v.emplace_back(lua.CreateLuaCallable(m).AcquireCallable());

    if constexpr (sizeof...(Methods) > 0)
    {
        emplace_methods(lua, v, methods...);
    }
}

template<typename ClassType, typename... Methods>
auto Lua::RegisterClassMultiString(std::string_view method_names, Methods... methods) -> void
{
    // split method names (as parameters) into vector, trim whitespace
    auto comma_separated = remove_all_whitespace(method_names);

    auto method_names_vector = split(comma_separated, std::string_view{","});

    std::vector<std::unique_ptr<ICallable>> methods_vector;
    methods_vector.reserve(sizeof...(Methods));
    emplace_methods(*this, methods_vector, methods...);

    RegisterClass<ClassType>(method_names_vector, std::move(methods_vector));
}

template<typename T>
auto destruct_shared_ptr_ptr(lua_State* state) -> int
{
    auto shared_ptr_ptr_ptr = static_cast<std::shared_ptr<T>**>(lua_touserdata(state, 1));

    delete *shared_ptr_ptr_ptr;

    return 0;
}

template<typename T, typename = void>
struct HasCreate : std::false_type
{
};

template<typename T>
struct HasCreate<T, std::void_t<decltype(T::Create)>> : std::true_type
{
};

template<typename ClassType, typename... Methods>
auto Lua::RegisterClass(std::vector<std::string_view> method_names, std::vector<std::unique_ptr<ICallable>> methods)
    -> void
{
    std::string                                                 class_name;
    auto                                                        name_pos = method_names.begin();
    std::unordered_map<std::string, std::unique_ptr<ICallable>> method_callables;

    for (auto& method : methods)
    {
        if (name_pos != method_names.end())
        {
            auto full_name = *name_pos;
            ++name_pos;

            auto last_scope = full_name.rfind("::");

            if (last_scope != std::string_view::npos)
            {
                auto method_class = full_name.substr(0, last_scope);
                auto method_name  = full_name.substr(last_scope + 2);

                // get rid of & from &ClassName::MethodName syntax
                if (method_class.size() > 0 && method_class.front() == '&')
                {
                    method_class.remove_prefix(1);
                }

                if (class_name.empty())
                {
                    class_name = std::string{method_class};
                }

                if (class_name == method_class)
                {
                    // add method to metatable
                    method_callables.emplace(std::string{method_name}, std::move(method));
                }
                else
                {
                    throw std::logic_error(
                        "RegisterClass had methods from various classes, only one class may be registered per call");
                }
            }
            else
            {
                throw std::logic_error("RegisterClass had method of invalid format, expecting ClassName::MethodName");
            }
        }
        else
        {
            throw std::logic_error("RegisterClass with fewer method names than methods!");
        }
    }

    // if we got here and have a class name then we had no expections, build Lua bindings
    if (!class_name.empty())
    {
        SetMetatableName<ClassType>(class_name);

        luaL_newmetatable(m_lua, class_name.data());

        lua_pushstring(m_lua, "__gc");
        lua_pushcfunction(m_lua, &destruct_shared_ptr_ptr<ClassType>);
        lua_settable(m_lua, -3);

        auto  pos_pair     = m_method_registry.emplace(class_name, std::move(method_callables));
        auto& our_registry = pos_pair.first->second;

        lua_pushstring(m_lua, "__index");
        lua_pushvalue(m_lua, -2);
        lua_settable(m_lua, -3);

        for (auto& method_pair : our_registry)
        {
            lua_pushstring(m_lua, method_pair.first.data());
            auto* callable_ptr = method_pair.second.get();
            lua_pushlightuserdata(m_lua, callable_ptr);
            lua_pushcclosure(m_lua, call_callable_from_lua, 1);

            lua_settable(m_lua, -3);
        }

        // pop the new metatable off the stack
        lua_pop(m_lua, 1);

        // now register the Create function if it exists
        if constexpr (HasCreate<ClassType>::value)
        {
            auto create_callable = CreateLuaCallable(&ClassType::Create);
            RegisterCallable("Create" + class_name, std::move(create_callable));
        }
    }
}

template<typename T>
auto Lua::GetMetatableName() -> std::optional<std::string>
{
    auto index = std::type_index{typeid(T)};

    auto pos = m_class_to_metatable_name.find(index);

    if (pos != m_class_to_metatable_name.end())
    {
        return pos->second;
    }
    else
    {
        return std::nullopt;
    }
}

template<typename T>
auto Lua::SetMetatableName(std::string metatable_name) -> void
{
    auto index = std::type_index{typeid(T)};

    m_class_to_metatable_name[index] = std::move(metatable_name);
}

} // namespace kdk::lua
