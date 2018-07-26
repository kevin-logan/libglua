#pragma once

#include "Exceptions.h"
#include "GluaCallable.h"
#include "ICallable.h"

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

#define REGISTER_TO_LUA(glua, functor) glua->RegisterCallable(#functor, glua->CreateLuaCallable(functor))

#define REGISTER_CLASS_TO_LUA(glua, ClassType, ...) glua->RegisterClassMultiString<ClassType>(#__VA_ARGS__, __VA_ARGS__)

namespace kdk::glua
{
class Glua : public std::enable_shared_from_this<Glua>
{
public:
    using Ptr = std::shared_ptr<Glua>;
    static auto Create(std::ostream& output_stream) -> Ptr;

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

    template<typename ClassType>
    auto RegisterMethod(const std::string& method_name, Callable method) -> void;

    auto GetLuaState() -> lua_State*;
    auto GetLuaState() const -> const lua_State*;

    template<typename T>
    auto GetMetatableName() -> std::optional<std::string>;

    template<typename T>
    auto SetMetatableName(std::string metatable_name) -> void;

    auto ResetEnvironment() -> void;

    ~Glua();

private:
    Glua(std::ostream& output_stream);

    auto get_global_onto_stack(const std::string& global_name) -> void;
    auto set_global_from_stack(const std::string& global_name) -> void;

    lua_State* m_lua;

    std::unordered_map<std::string, std::unique_ptr<ICallable>>                                  m_registry;
    std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<ICallable>>> m_method_registry;
    std::unordered_map<std::type_index, std::string> m_class_to_metatable_name;

    std::ostream& m_output_stream;
};

auto call_callable_from_lua(lua_State* state) -> int;
auto destruct_managed_type(lua_State* state) -> int;

auto remove_all_whitespace(std::string_view input) -> std::string;
auto split(std::string_view input, std::string_view token) -> std::vector<std::string_view>;

// specialize this type with your type to add custom handling
template<typename T, typename = void>
struct ManagedTypeHandler
{
    static auto get(lua_State* state, size_t stack_index) -> T
    {
        (void)state;
        (void)stack_index;
        throw exceptions::LuaException("Tried to get value of unmanaged type");
    }
    static auto push(lua_State* state, T value) -> void
    {
        (void)state;
        (void)value;
        throw exceptions::LuaException("Tried to push value of unmanaged type");
    }
};

template<typename T>
struct IsVector : std::false_type
{
};

template<typename T>
struct IsVector<std::vector<T>> : std::true_type
{
};

template<typename T>
struct IsOptional : std::false_type
{
};

template<typename T>
struct IsOptional<std::optional<T>> : std::true_type
{
};

template<typename T>
struct IsSharedPtr : std::false_type
{
};

template<typename T>
struct IsSharedPtr<std::shared_ptr<T>> : std::true_type
{
};

template<typename T>
struct IsReferenceWrapper : std::false_type
{
};

template<typename T>
struct IsReferenceWrapper<std::reference_wrapper<T>> : std::true_type
{
};

template<typename T>
struct HasTypeHandler
{
    static constexpr bool value =
        IsVector<T>::value || IsOptional<T>::value || IsSharedPtr<T>::value || IsReferenceWrapper<T>::value;
};

template<typename T, typename = void>
struct HasCustomGet : std::false_type
{
};

template<typename T>
struct HasCustomGet<T, std::void_t<decltype(T::glua_get(std::declval<lua_State*>(), std::declval<size_t>()))>>
    : std::true_type
{
};

template<typename T, typename = void>
struct HasCustomPush : std::false_type
{
};

template<typename T>
struct HasCustomPush<T, std::void_t<decltype(T::glua_push(std::declval<lua_State*>(), std::declval<T>()))>>
    : std::true_type
{
};

} // namespace kdk::glua

#include "Glua.tcc"
