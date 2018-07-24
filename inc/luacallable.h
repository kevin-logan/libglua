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
    auto Push(const std::vector<T>& vector) -> void;

    template<typename T>
    auto Push(T value) -> void;

    template<typename T>
    auto PopArray() -> std::vector<T>;

    template<typename T>
    auto Pop() -> T;

    template<typename T>
    auto SetGlobal(const std::string& name, T value) -> void;

    template<typename T>
    auto GetGlobal(const std::string& name) -> T;

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
auto destruct_managed_type(lua_State* state) -> int;

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

enum class ManagedTypeStorage
{
    RAW_PTR,
    SHARED_PTR,
    STACK_ALLOCATED
};

class IManagedTypeStorage
{
public:
    virtual auto GetStorageType() const -> ManagedTypeStorage = 0;

    virtual ~IManagedTypeStorage() = default;
};

template<typename T>
class ManagedTypeSharedPtr : public IManagedTypeStorage
{
public:
    ManagedTypeSharedPtr(std::shared_ptr<T> value) : m_value(value) {}

    auto GetStorageType() const -> ManagedTypeStorage override { return ManagedTypeStorage::SHARED_PTR; }

    auto GetValue() const -> std::shared_ptr<const T> { return m_value; }
    auto GetValue() -> std::shared_ptr<T> { return m_value; }

    ~ManagedTypeSharedPtr() override = default;

private:
    std::shared_ptr<T> m_value;
};

template<typename T>
class ManagedTypeRawPtr : public IManagedTypeStorage
{
public:
    ManagedTypeRawPtr(T* value) : m_value(value) {}

    auto GetStorageType() const -> ManagedTypeStorage override { return ManagedTypeStorage::RAW_PTR; }

    auto GetValue() const -> const T* { return m_value; }
    auto GetValue() -> T* { return m_value; }

    ~ManagedTypeRawPtr() override = default; // we don't own the object, so we don't delete it

private:
    T* m_value;
};

template<typename T>
class ManagedTypeStackAllocated : public IManagedTypeStorage
{
public:
    ManagedTypeStackAllocated(T value) : m_value(std::move(value)) {}

    auto GetStorageType() const -> ManagedTypeStorage override { return ManagedTypeStorage::STACK_ALLOCATED; }

    auto GetValue() const -> const T& { return m_value; }
    auto GetValue() -> T& { return m_value; }

    ~ManagedTypeStackAllocated() override = default;

private:
    T m_value;
};

template<typename T, typename = void>
struct ManagedTypeHandler
{
    static auto get(lua_State* state, size_t stack_index) -> T
    {
        if constexpr (std::is_copy_constructible<T>::value)
        {
            lua_getglobal(state, "LuaClass");
            auto* lua_object = static_cast<Lua*>(lua_touserdata(state, -1));
            lua_pop(state, 1);

            auto metatable_name_opt = lua_object->GetMetatableName<T>();

            if (metatable_name_opt.has_value())
            {
                IManagedTypeStorage* managed_type_ptr = static_cast<IManagedTypeStorage*>(
                    luaL_checkudata(state, static_cast<int>(stack_index), metatable_name_opt.value().data()));

                if (managed_type_ptr != nullptr)
                {
                    switch (managed_type_ptr->GetStorageType())
                    {
                        case ManagedTypeStorage::RAW_PTR:
                        {
                            return *static_cast<ManagedTypeRawPtr<T>*>(managed_type_ptr)->GetValue();
                        }
                        break;
                        case ManagedTypeStorage::SHARED_PTR:
                        {
                            return *static_cast<ManagedTypeSharedPtr<T>*>(managed_type_ptr)->GetValue();
                        }
                        break;
                        case ManagedTypeStorage::STACK_ALLOCATED:
                        {
                            return static_cast<ManagedTypeStackAllocated<T>*>(managed_type_ptr)->GetValue();
                        }
                        break;
                    }
                }
                else
                {
                    throw kdk::exceptions::LuaException("Tried to get type but invalid value at index!");
                }
            }
            else
            {
                throw kdk::exceptions::LuaException("Tried to get type with no registered metatable!");
            }
        }
        else
        {
            throw exceptions::LuaException("Tried to get value of unmanaged type");
        }
    }

    static auto push(lua_State* state, T value) -> void
    {
        if constexpr (std::is_copy_constructible<T>::value)
        {
            lua_getglobal(state, "LuaClass");
            auto* lua_object = static_cast<Lua*>(lua_touserdata(state, -1));
            lua_pop(state, 1);

            auto metatable_name_opt = lua_object->GetMetatableName<T>();

            if (metatable_name_opt.has_value())
            {
                ManagedTypeStackAllocated<T>* managed_type_ptr = static_cast<ManagedTypeStackAllocated<T>*>(
                    lua_newuserdata(state, sizeof(ManagedTypeStackAllocated<T>)));

                managed_type_ptr = new (managed_type_ptr) ManagedTypeStackAllocated<T>{std::move(value)};

                auto metatable_name = metatable_name_opt.value();

                // set the metatable for this object
                luaL_getmetatable(state, metatable_name.data());

                if (lua_isnil(state, -1))
                {
                    lua_pop(state, 1);
                    managed_type_ptr->~ManagedTypeStackAllocated<T>();

                    throw std::runtime_error(
                        "Pushing class type which has not been registered [null metatable]! " + metatable_name);
                }
                lua_setmetatable(state, -2);
            }
            else
            {
                throw std::runtime_error("Pushing class type which has not been registered [no metatable name]!");
            }
        }
        else
        {
            throw exceptions::LuaException("Tried to push value of unmanaged type");
        }
    }
}; // namespace kdk::lua

template<typename T>
struct ManagedTypeHandler<T*, void> // std::void_t<decltype(std::declval<T*>()->shared_from_this())>>
{
    static auto get(lua_State* state, int32_t stack_index) -> T*
    {
        lua_getglobal(state, "LuaClass");
        auto* lua_object = static_cast<Lua*>(lua_touserdata(state, -1));
        lua_pop(state, 1);

        auto metatable_name_opt = lua_object->GetMetatableName<T>();

        if (metatable_name_opt.has_value())
        {
            IManagedTypeStorage* managed_type_ptr = static_cast<IManagedTypeStorage*>(
                luaL_checkudata(state, static_cast<int>(stack_index), metatable_name_opt.value().data()));

            if (managed_type_ptr != nullptr)
            {
                switch (managed_type_ptr->GetStorageType())
                {
                    case ManagedTypeStorage::RAW_PTR:
                    {
                        return static_cast<ManagedTypeRawPtr<T>*>(managed_type_ptr)->GetValue();
                    }
                    break;
                    case ManagedTypeStorage::SHARED_PTR:
                    {
                        return static_cast<ManagedTypeSharedPtr<T>*>(managed_type_ptr)->GetValue().get();
                    }
                    break;
                    case ManagedTypeStorage::STACK_ALLOCATED:
                    {
                        return &static_cast<ManagedTypeStackAllocated<T>*>(managed_type_ptr)->GetValue();
                    }
                    break;
                }
            }
            else
            {
                throw kdk::exceptions::LuaException("Tried to get type but invalid value at index!");
            }
        }
        else
        {
            throw kdk::exceptions::LuaException("Tried to get type with no registered metatable!");
        }
    }

    static auto push(lua_State* state, T* value) -> void
    {
        lua_getglobal(state, "LuaClass");
        auto* lua_object = static_cast<Lua*>(lua_touserdata(state, -1));
        lua_pop(state, 1);

        auto metatable_name_opt = lua_object->GetMetatableName<T>();

        if (metatable_name_opt.has_value())
        {
            ManagedTypeRawPtr<T>* managed_type_ptr =
                static_cast<ManagedTypeRawPtr<T>*>(lua_newuserdata(state, sizeof(ManagedTypeRawPtr<T>)));

            managed_type_ptr = new (managed_type_ptr) ManagedTypeRawPtr<T>{value};

            auto metatable_name = metatable_name_opt.value();

            // set the metatable for this object
            luaL_getmetatable(state, metatable_name.data());

            if (lua_isnil(state, -1))
            {
                lua_pop(state, 1);
                managed_type_ptr->~ManagedTypeRawPtr<T>();

                throw std::runtime_error(
                    "Pushing class type which has not been registered [null metatable]! " + metatable_name);
            }
            lua_setmetatable(state, -2);
        }
        else
        {
            throw std::runtime_error("Pushing class type which has not been registered [no metatable name]!");
        }
    }
};

template<typename T>
struct ManagedTypeHandler<T&, void> // std::void_t<decltype(std::declval<T&>().shared_from_this())>>
{
    static auto get(lua_State* state, int32_t stack_index) -> T&
    {
        lua_getglobal(state, "LuaClass");
        auto* lua_object = static_cast<Lua*>(lua_touserdata(state, -1));
        lua_pop(state, 1);

        auto metatable_name_opt = lua_object->GetMetatableName<T>();

        if (metatable_name_opt.has_value())
        {
            IManagedTypeStorage* managed_type_ptr = static_cast<IManagedTypeStorage*>(
                luaL_checkudata(state, static_cast<int>(stack_index), metatable_name_opt.value().data()));

            if (managed_type_ptr != nullptr)
            {
                switch (managed_type_ptr->GetStorageType())
                {
                    case ManagedTypeStorage::RAW_PTR:
                    {
                        return *static_cast<ManagedTypeRawPtr<T>*>(managed_type_ptr)->GetValue();
                    }
                    break;
                    case ManagedTypeStorage::SHARED_PTR:
                    {
                        return *static_cast<ManagedTypeSharedPtr<T>*>(managed_type_ptr)->GetValue();
                    }
                    break;
                    case ManagedTypeStorage::STACK_ALLOCATED:
                    {
                        return static_cast<ManagedTypeStackAllocated<T>*>(managed_type_ptr)->GetValue();
                    }
                    break;
                }
            }
            else
            {
                throw kdk::exceptions::LuaException("Tried to get type but invalid value at index!");
            }
        }
        else
        {
            throw kdk::exceptions::LuaException("Tried to get type with no registered metatable!");
        }
    }

    static auto push(lua_State* state, T& value) -> void
    {
        lua_getglobal(state, "LuaClass");
        auto* lua_object = static_cast<Lua*>(lua_touserdata(state, -1));
        lua_pop(state, 1);

        auto metatable_name_opt = lua_object->GetMetatableName<T>();

        if (metatable_name_opt.has_value())
        {
            ManagedTypeRawPtr<T>* managed_type_ptr =
                static_cast<ManagedTypeRawPtr<T>*>(lua_newuserdata(state, sizeof(ManagedTypeRawPtr<T>)));

            managed_type_ptr = new (managed_type_ptr) ManagedTypeRawPtr<T>{&value};

            auto metatable_name = metatable_name_opt.value();

            // set the metatable for this object
            luaL_getmetatable(state, metatable_name.data());

            if (lua_isnil(state, -1))
            {
                lua_pop(state, 1);
                managed_type_ptr->~ManagedTypeRawPtr<T>();

                throw std::runtime_error(
                    "Pushing class type which has not been registered [null metatable]! " + metatable_name);
            }
            lua_setmetatable(state, -2);
        }
        else
        {
            throw std::runtime_error("Pushing class type which has not been registered [no metatable name]!");
        }
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

        if (metatable_name_opt.has_value())
        {
            IManagedTypeStorage* managed_type_ptr = static_cast<IManagedTypeStorage*>(
                luaL_checkudata(state, static_cast<int>(stack_index), metatable_name_opt.value().data()));

            if (managed_type_ptr != nullptr)
            {
                if (managed_type_ptr->GetStorageType() == ManagedTypeStorage::SHARED_PTR)
                {
                    return static_cast<ManagedTypeSharedPtr<T>*>(managed_type_ptr)->GetValue();
                }
                else
                {
                    throw kdk::exceptions::LuaException("Tried to get shared pointer of type but invalid value at index!");
                }
            }
            else
            {
                throw kdk::exceptions::LuaException("Tried to get type but invalid value at index!");
            }
        }
        else
        {
            throw kdk::exceptions::LuaException("Tried to get type with no registered metatable!");
        }
    }

    static auto push(lua_State* state, std::shared_ptr<T> value) -> void
    {
        lua_getglobal(state, "LuaClass");
        auto* lua_object = static_cast<Lua*>(lua_touserdata(state, -1));
        lua_pop(state, 1);

        auto metatable_name_opt = lua_object->GetMetatableName<T>();

        if (metatable_name_opt.has_value())
        {
            ManagedTypeSharedPtr<T>* managed_type_ptr =
                static_cast<ManagedTypeSharedPtr<T>*>(lua_newuserdata(state, sizeof(ManagedTypeSharedPtr<T>)));

            managed_type_ptr = new (managed_type_ptr) ManagedTypeSharedPtr<T>{value};

            auto metatable_name = metatable_name_opt.value();

            // set the metatable for this object
            luaL_getmetatable(state, metatable_name.data());

            if (lua_isnil(state, -1))
            {
                lua_pop(state, 1);
                managed_type_ptr->~ManagedTypeSharedPtr<T>();

                throw std::runtime_error(
                    "Pushing class type which has not been registered [null metatable]! " + metatable_name);
            }
            lua_setmetatable(state, -2);
        }
        else
        {
            throw std::runtime_error("Pushing class type which has not been registered [no metatable name]!");
        }
    }
};

template<typename T>
struct ManagedTypeHandler<std::reference_wrapper<T>, void>
{
    static auto get(lua_State* state, int32_t stack_index) -> std::reference_wrapper<T>
    {
        lua_getglobal(state, "LuaClass");
        auto* lua_object = static_cast<Lua*>(lua_touserdata(state, -1));
        lua_pop(state, 1);

        auto metatable_name_opt = lua_object->GetMetatableName<T>();

        if (metatable_name_opt.has_value())
        {
            IManagedTypeStorage* managed_type_ptr = static_cast<IManagedTypeStorage*>(
                luaL_checkudata(state, static_cast<int>(stack_index), metatable_name_opt.value().data()));

            if (managed_type_ptr != nullptr)
            {
                switch (managed_type_ptr->GetStorageType())
                {
                    case ManagedTypeStorage::RAW_PTR:
                    {
                        return std::ref(*static_cast<ManagedTypeRawPtr<T>*>(managed_type_ptr)->GetValue());
                    }
                    break;
                    case ManagedTypeStorage::SHARED_PTR:
                    {
                        return std::ref(*static_cast<ManagedTypeSharedPtr<T>*>(managed_type_ptr)->GetValue());
                    }
                    break;
                    case ManagedTypeStorage::STACK_ALLOCATED:
                    {
                        return std::ref(static_cast<ManagedTypeStackAllocated<T>*>(managed_type_ptr)->GetValue());
                    }
                    break;
                }
            }
            else
            {
                throw kdk::exceptions::LuaException("Tried to get type but invalid value at index!");
            }
        }
        else
        {
            throw kdk::exceptions::LuaException("Tried to get type with no registered metatable!");
        }
    }

    static auto push(lua_State* state, std::reference_wrapper<T> value) -> void
    {
        lua_getglobal(state, "LuaClass");
        auto* lua_object = static_cast<Lua*>(lua_touserdata(state, -1));
        lua_pop(state, 1);

        auto metatable_name_opt = lua_object->GetMetatableName<T>();

        if (metatable_name_opt.has_value())
        {
            ManagedTypeRawPtr<T>* managed_type_ptr =
                static_cast<ManagedTypeRawPtr<T>*>(lua_newuserdata(state, sizeof(ManagedTypeRawPtr<T>)));

            managed_type_ptr = new (managed_type_ptr) ManagedTypeRawPtr<T>{&value.get()};

            auto metatable_name = metatable_name_opt.value();

            // set the metatable for this object
            luaL_getmetatable(state, metatable_name.data());

            if (lua_isnil(state, -1))
            {
                lua_pop(state, 1);
                managed_type_ptr->~ManagedTypeRawPtr<T>();

                throw std::runtime_error(
                    "Pushing class type which has not been registered [null metatable]! " + metatable_name);
            }
            lua_setmetatable(state, -2);
        }
        else
        {
            throw std::runtime_error("Pushing class type which has not been registered [no metatable name]!");
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
auto Lua::Push(const std::vector<T>& arr) -> void
{
    auto size = arr.size();
    lua_createtable(m_lua, size, 0);
    for (size_t i = 0; i < size; ++i)
    {
        lua_pushinteger(m_lua, static_cast<lua_Integer>(i + 1)); // lua arrays 1 based
        Push(arr[i]);

        lua_settable(m_lua, -3);
    }
}

template<typename T>
auto Lua::Push(T value) -> void
{
    return ManagedTypeHandler<T>::push(m_lua, std::move(value));
}

template<typename T>
auto Lua::PopArray() -> std::vector<T>
{
    if (lua_istable(m_lua, -1))
    {
        std::vector<T> result;

        size_t length = lua_objlen(m_lua, -1);

        result.reserve(length);

        for (size_t i = 0; i < length; ++i)
        {
            // get our value onto the stack...
            lua_pushinteger(m_lua, static_cast<lua_Integer>(i + 1)); // lua arrays 1 based
            lua_gettable(m_lua, -2);

            result.emplace_back(Pop<T>());
        }

        return result;
    }
    else
    {
        throw exceptions::LuaException("Attempted to pop array off stack but top of stack wasn't a table");
    }
}

template<typename T>
auto Lua::Pop() -> T
{
    auto val = get<T>(m_lua, -1);
    lua_pop(m_lua, 1);

    return val;
}

template<typename T>
auto Lua::SetGlobal(const std::string& name, T value) -> void
{
    Push(std::move(value));
    lua_setglobal(m_lua, name.data());
}

template<typename T>
auto Lua::GetGlobal(const std::string& name) -> T
{
    lua_getglobal(m_lua, name.data());
    return Pop<T>();
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

template<typename T, typename = void>
struct HasCreate : std::false_type
{
};

template<typename T>
struct HasCreate<T, std::void_t<decltype(T::Create)>> : std::true_type
{
};

template<typename T>
auto default_construct_type() -> T
{
    return T{};
}

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
        lua_pushcfunction(m_lua, &destruct_managed_type);
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

        if constexpr (std::is_default_constructible<ClassType>::value)
        {
            auto construct_callable = CreateLuaCallable(&default_construct_type<ClassType>);
            RegisterCallable("Construct" + class_name, std::move(construct_callable));
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
