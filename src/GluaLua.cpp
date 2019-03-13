#include "glua/GluaLua.h"
#include "glua/FileUtil.h"

#include <iostream>

namespace kdk::glua {
auto LuaStateDeleter::operator()(lua_State* state) -> void
{
    if (state != nullptr) {
        lua_close(state);
    }
}

static auto glua_capture_print(lua_State* lua) -> int
{
    auto arg_count = lua_gettop(lua);
    auto& ostream = *static_cast<std::ostream*>(lua_touserdata(lua, lua_upvalueindex(1)));

    for (auto i = 1; i <= arg_count; ++i) {
        ostream << lua_tostring(lua, i);
    }

    ostream << std::endl; // print ends with newline

    return 0;
}

static auto glua_set_environment_to_stack(
    lua_State* lua, const std::vector<std::string>& environment) -> void
{
    // table we're setting is currently top  (-1) on stack

    for (const auto& str : environment) {
        lua_pushlstring(lua, str.data(), str.length());
        lua_getglobal(lua, str.data());
        lua_settable(lua, -3);
    }

    // set global env value to same table
    lua_pushliteral(lua, "_G");
    lua_pushvalue(lua, -2);
    lua_settable(lua, -3);
}

static auto
glua_set_sub_environment_to_stack(lua_State* lua, std::string_view parent_table, const std::vector<std::string>& environment) -> void
{
    // table we're setting is currently top  (-1) on stack

    lua_pushlstring(lua, parent_table.data(), parent_table.size());
    lua_createtable(lua, 0, static_cast<int>(environment.size()));

    for (const auto& str : environment) {
        lua_pushlstring(lua, str.data(), str.length());

        lua_getglobal(lua, parent_table.data());
        lua_pushlstring(lua, str.data(), str.length());
        lua_gettable(lua, -2);

        lua_remove(lua, -2); // remove parent table from stack

        lua_settable(lua, -3);
    }

    lua_settable(lua, -3);
}

GluaLua::GluaLua(std::ostream& output_stream, bool start_sandboxed)
    : m_lua(luaL_newstate())
    , m_output_stream(output_stream)
    , m_current_array_index(0)
{
    luaL_openlibs(m_lua.get());

    luaL_Reg print_override_lib[] = { { "print", glua_capture_print },
        { nullptr, nullptr } };

    // create sandbox environment
    lua_newtable(m_lua.get());

    std::vector<std::string> sandbox_environment {
        "assert", "error", "ipairs", "next", "pairs",
        "pcall", "print", "select", "tonumber", "tostring",
        "type", "unpack", "_VERSION", "xpcall", "isfunction"
    };
    std::vector<std::string> sandbox_coroutine_environment {
        "create", "resume", "running", "status", "wrap", "yield"
    };
    std::vector<std::string> sandbox_io_environment { "read", "write", "flush",
        "type" };
    std::vector<std::string> sandbox_string_environment {
        "byte", "char", "dump", "find", "format", "gmatch", "gsub",
        "len", "lower", "match", "rep", "reverse", "sub", "upper"
    };
    std::vector<std::string> sandbox_table_environment { "insert", "maxn", "remove",
        "sort" };
    std::vector<std::string> sandbox_math_environment {
        "abs", "acos", "asin", "atan", "atan2", "ceil", "cos", "cosh",
        "deg", "exp", "floor", "fmod", "frexp", "huge", "ldexp", "log",
        "log10", "max", "min", "modf", "pi", "pow", "rad", "random",
        "sin", "sinh", "sqrt", "tan", "tanh"
    };
    std::vector<std::string> sandbox_os_environment { "clock", "difftime", "time" };

    glua_set_environment_to_stack(m_lua.get(), sandbox_environment);
    glua_set_sub_environment_to_stack(m_lua.get(), "coroutine",
        sandbox_coroutine_environment);
    glua_set_sub_environment_to_stack(m_lua.get(), "io", sandbox_io_environment);
    glua_set_sub_environment_to_stack(m_lua.get(), "string",
        sandbox_string_environment);
    glua_set_sub_environment_to_stack(m_lua.get(), "table",
        sandbox_table_environment);
    glua_set_sub_environment_to_stack(m_lua.get(), "math",
        sandbox_math_environment);
    glua_set_sub_environment_to_stack(m_lua.get(), "os", sandbox_os_environment);

    lua_pushlightuserdata(m_lua.get(), &m_output_stream.get());
    luaL_setfuncs(m_lua.get(), print_override_lib, 1);

    lua_setglobal(m_lua.get(), "__libglua__sandbox__");

    // now that sandbox is set up, reset environment
    ResetEnvironment(start_sandboxed);

    // put print override on the non-sandbox _G as well
    lua_getglobal(m_lua.get(), "_G");
    lua_pushlightuserdata(m_lua.get(), &m_output_stream.get());
    luaL_setfuncs(m_lua.get(), print_override_lib, 1);
    lua_pop(m_lua.get(), 1);

    lua_pushlightuserdata(m_lua.get(), this);
    lua_setglobal(m_lua.get(), "LuaClass");
}
auto GluaLua::GetInstanceFromState(lua_State* lua) -> GluaLua&
{
    lua_getglobal(lua, "LuaClass");
    auto* lua_object = static_cast<kdk::glua::GluaLua*>(lua_touserdata(lua, -1));
    lua_pop(lua, 1);

    return *lua_object;
}
auto GluaLua::ResetEnvironment(bool sandboxed) -> void
{
    if (sandboxed) {
        lua_getglobal(m_lua.get(), "__libglua__sandbox__");
    } else {
        lua_getglobal(m_lua.get(), "_G");
    }

    lua_newtable(m_lua.get()); // env
    lua_pushvalue(m_lua.get(), -1); // env -> env
    lua_pushliteral(m_lua.get(), "__index"); // env -> env -> __index
    lua_pushvalue(m_lua.get(), -4); // push (__libglua__sandbox__ or _G), env ->
        // env -> __index -> sandbox
    lua_settable(m_lua.get(), -3); // env -> env
    lua_setmetatable(m_lua.get(), -2); // env

    lua_setglobal(m_lua.get(), "__libglua__env__");
}
auto GluaLua::RegisterCallable(const std::string& name, Callable callable)
    -> void
{
    auto insert_pair = m_registry.emplace(std::string { name },
        std::move(callable).AcquireCallable());

    if (insert_pair.second) {
        auto* callable_ptr = insert_pair.first->second.get();
        lua_pushlightuserdata(m_lua.get(), callable_ptr);
        lua_pushcclosure(m_lua.get(), call_callable_from_lua, 1);

        // set the closure on the sandbox environment
        lua_getglobal(m_lua.get(), "__libglua__sandbox__");
        lua_pushlstring(m_lua.get(), name.data(), name.size());
        lua_pushvalue(m_lua.get(), -3); // push closer back on stack
        lua_settable(m_lua.get(), -3);
        lua_pop(m_lua.get(), 1);

        // now the original closure is all that's left on the stack, set to global
        // env too
        lua_setglobal(m_lua.get(), name.data());
    } else {
        throw exceptions::LuaException(
            "Registered a callable with an already used name");
    }
}
auto GluaLua::push(std::nullopt_t /*unused*/) -> void
{
    lua_pushnil(m_lua.get());
}
auto GluaLua::push(bool value) -> void
{
    lua_pushboolean(m_lua.get(), value ? 1 : 0);
}
auto GluaLua::push(int8_t value) -> void
{
    lua_pushinteger(m_lua.get(), value);
}
auto GluaLua::push(int16_t value) -> void
{
    lua_pushinteger(m_lua.get(), value);
}
auto GluaLua::push(int32_t value) -> void
{
    lua_pushinteger(m_lua.get(), value);
}
auto GluaLua::push(int64_t value) -> void
{
    lua_pushinteger(m_lua.get(), value);
}
auto GluaLua::push(uint8_t value) -> void
{
    lua_pushinteger(m_lua.get(), static_cast<lua_Integer>(value));
}
auto GluaLua::push(uint16_t value) -> void
{
    lua_pushinteger(m_lua.get(), static_cast<lua_Integer>(value));
}
auto GluaLua::push(uint32_t value) -> void
{
    lua_pushinteger(m_lua.get(), static_cast<lua_Integer>(value));
}
auto GluaLua::push(uint64_t value) -> void
{
    lua_pushinteger(m_lua.get(), static_cast<lua_Integer>(value));
}
auto GluaLua::push(float value) -> void
{
    lua_pushnumber(m_lua.get(), static_cast<double>(value));
}
auto GluaLua::push(double value) -> void { lua_pushnumber(m_lua.get(), value); }
auto GluaLua::push(const char* value) -> void
{
    lua_pushstring(m_lua.get(), value);
}
auto GluaLua::push(std::string_view value) -> void
{
    lua_pushlstring(m_lua.get(), value.data(), value.size());
}
auto GluaLua::push(std::string value) -> void
{
    lua_pushlstring(m_lua.get(), value.data(), value.size());
}
auto GluaLua::pushArray(size_t size_hint) -> void
{
    lua_createtable(m_lua.get(), static_cast<int>(size_hint), 0);
}
auto GluaLua::pushStartMap(size_t size_hint) -> void
{
    lua_createtable(m_lua.get(), 0, static_cast<int>(size_hint));
}
auto GluaLua::arraySetFromStack() -> void
{
    // top of stack: value
    // top -1: index
    // top -2: table
    lua_settable(m_lua.get(), -3);
}
auto GluaLua::mapSetFromStack() -> void
{
    // top of stack: value
    // top -1: key
    // top -2: table
    lua_settable(m_lua.get(), -3);
}
auto GluaLua::pushUserType(const std::string& unique_type_name,
    std::unique_ptr<IManagedTypeStorage> user_storage)
    -> void
{
    auto** managed_type_ptr = static_cast<IManagedTypeStorage**>(
        lua_newuserdata(m_lua.get(), sizeof(IManagedTypeStorage*)));

    *managed_type_ptr = user_storage.get();
    user_storage.release();

    // set the metatable for this object
    luaL_getmetatable(m_lua.get(), unique_type_name.data());

    if (lua_isnil(m_lua.get(), -1)) {
        lua_pop(m_lua.get(), 1);

        // reacquire the pointer as a unique pointer to delete it when leaving scope
        std::unique_ptr<IManagedTypeStorage> reacquired_memory { *managed_type_ptr };

        throw std::runtime_error(
            "Pushing class type which has not been registered [null metatable]! " + unique_type_name);
    }

    lua_setmetatable(m_lua.get(), -2);
}
auto GluaLua::getBool(int stack_index) const -> bool
{
    return static_cast<bool>(lua_toboolean(m_lua.get(), stack_index));
}
auto GluaLua::getInt8(int stack_index) const -> int8_t
{
    return static_cast<int8_t>(lua_tointeger(m_lua.get(), stack_index));
}
auto GluaLua::getInt16(int stack_index) const -> int16_t
{
    return static_cast<int16_t>(lua_tointeger(m_lua.get(), stack_index));
}
auto GluaLua::getInt32(int stack_index) const -> int32_t
{
    return static_cast<int32_t>(lua_tointeger(m_lua.get(), stack_index));
}
auto GluaLua::getInt64(int stack_index) const -> int64_t
{
    return static_cast<int64_t>(lua_tointeger(m_lua.get(), stack_index));
}
auto GluaLua::getUInt8(int stack_index) const -> uint8_t
{
    return static_cast<uint8_t>(lua_tointeger(m_lua.get(), stack_index));
}
auto GluaLua::getUInt16(int stack_index) const -> uint16_t
{
    return static_cast<uint16_t>(lua_tointeger(m_lua.get(), stack_index));
}
auto GluaLua::getUInt32(int stack_index) const -> uint32_t
{
    return static_cast<uint32_t>(lua_tointeger(m_lua.get(), stack_index));
}
auto GluaLua::getUInt64(int stack_index) const -> uint64_t
{
    return static_cast<uint64_t>(lua_tointeger(m_lua.get(), stack_index));
}
auto GluaLua::getFloat(int stack_index) const -> float
{
    return static_cast<float>(lua_tonumber(m_lua.get(), stack_index));
}
auto GluaLua::getDouble(int stack_index) const -> double
{
    return static_cast<double>(lua_tonumber(m_lua.get(), stack_index));
}
auto GluaLua::getCharPointer(int stack_index) const -> const char*
{
    return lua_tostring(m_lua.get(), stack_index);
}
auto GluaLua::getStringView(int stack_index) const -> std::string_view
{
    size_t length;
    const auto* c_str = lua_tolstring(m_lua.get(), stack_index, &length);

    std::string_view ret_val { c_str, length };

    return ret_val;
}
auto GluaLua::getString(int stack_index) const -> std::string
{
    size_t length;
    const auto* c_str = lua_tolstring(m_lua.get(), stack_index, &length);

    std::string ret_val { c_str, length };

    return ret_val;
}
auto GluaLua::getArraySize(int stack_index) const -> size_t
{
    if (lua_istable(m_lua.get(), stack_index)) {
        return lua_objlen(m_lua.get(), stack_index);
    }

    throw exceptions::LuaException("GetArraySize for non-table value");
}
auto GluaLua::getArrayValue(size_t index_into_array,
    int stack_index_of_array) const -> void
{
    auto absolute_array_index = absoluteIndex(stack_index_of_array);
    lua_pushinteger(m_lua.get(), static_cast<lua_Integer>(index_into_array));
    lua_gettable(m_lua.get(), absolute_array_index);
}
auto GluaLua::getMapKeys(int stack_index) const -> std::vector<std::string>
{
    auto absolute_map_index = absoluteIndex(stack_index);
    if (lua_istable(m_lua.get(), stack_index)) {
        auto size = lua_objlen(m_lua.get(), stack_index);

        std::vector<std::string> result;
        result.reserve(size);

        // push first nil key to start iteration
        lua_pushnil(m_lua.get());
        while (lua_next(m_lua.get(), absolute_map_index) != 0) {
            // key then value were pushed onto stack
            lua_pop(m_lua.get(), 1); // pop off value, we're not looking

            if (lua_isstring(m_lua.get(), -1) != 0) {
                size_t str_len = 0;
                const char* str = lua_tolstring(m_lua.get(), -1, &str_len);
                result.emplace_back(str, str_len);
            } else {
                // key isn't string, lua_tolstring will convert it to a string and mess
                // up iteration, create copy
                lua_pushvalue(m_lua.get(), -1);

                size_t str_len = 0;
                const char* str = lua_tolstring(m_lua.get(), -1, &str_len);
                result.emplace_back(str, str_len);

                // now pop off copy
                lua_pop(m_lua.get(), 1);
            }
        }

        return result;
    }

    throw exceptions::LuaException("GetMapKeys for non-table value");
}
auto GluaLua::getMapValue(const std::string& key, int stack_index_of_map) const
    -> void
{
    auto absolute_map_index = absoluteIndex(stack_index_of_map);
    lua_pushlstring(m_lua.get(), key.data(), key.size());
    lua_gettable(m_lua.get(), absolute_map_index);
}
auto GluaLua::getUserType(const std::string& unique_type_name,
    int stack_index) const -> IManagedTypeStorage*
{
    auto** managed_type_ptr = static_cast<IManagedTypeStorage**>(
        luaL_checkudata(m_lua.get(), stack_index, unique_type_name.data()));

    if (managed_type_ptr != nullptr) {
        return *managed_type_ptr;
    }

    throw exceptions::LuaException(
        "Tried to get type but invalid value at index!");
}
auto GluaLua::isUserType(const std::string& unique_type_name,
    int stack_index) const -> bool
{
    return luaL_checkudata(m_lua.get(), stack_index, unique_type_name.data()) != nullptr;
}
auto GluaLua::isNull(int stack_index) const -> bool
{
    return lua_isnil(m_lua.get(), stack_index) != 0;
}
auto GluaLua::isBool(int stack_index) const -> bool
{
    return lua_isboolean(m_lua.get(), stack_index) != 0;
}
auto GluaLua::isInt8(int stack_index) const -> bool
{
    return lua_isnumber(m_lua.get(), stack_index) != 0;
}
auto GluaLua::isInt16(int stack_index) const -> bool
{
    return lua_isnumber(m_lua.get(), stack_index) != 0;
}
auto GluaLua::isInt32(int stack_index) const -> bool
{
    return lua_isnumber(m_lua.get(), stack_index) != 0;
}
auto GluaLua::isInt64(int stack_index) const -> bool
{
    return lua_isnumber(m_lua.get(), stack_index) != 0;
}
auto GluaLua::isUInt8(int stack_index) const -> bool
{
    return lua_isnumber(m_lua.get(), stack_index) != 0;
}
auto GluaLua::isUInt16(int stack_index) const -> bool
{
    return lua_isnumber(m_lua.get(), stack_index) != 0;
}
auto GluaLua::isUInt32(int stack_index) const -> bool
{
    return lua_isnumber(m_lua.get(), stack_index) != 0;
}
auto GluaLua::isUInt64(int stack_index) const -> bool
{
    return lua_isnumber(m_lua.get(), stack_index) != 0;
}
auto GluaLua::isFloat(int stack_index) const -> bool
{
    return lua_isnumber(m_lua.get(), stack_index) != 0;
}
auto GluaLua::isDouble(int stack_index) const -> bool
{
    return lua_isnumber(m_lua.get(), stack_index) != 0;
}
auto GluaLua::isCharPointer(int stack_index) const -> bool
{
    return lua_isstring(m_lua.get(), stack_index) != 0;
}
auto GluaLua::isStringView(int stack_index) const -> bool
{
    return lua_isstring(m_lua.get(), stack_index) != 0;
}
auto GluaLua::isString(int stack_index) const -> bool
{
    return lua_isstring(m_lua.get(), stack_index) != 0;
}
auto GluaLua::isArray(int stack_index) const -> bool
{
    return lua_istable(m_lua.get(), stack_index) != 0;
}
auto GluaLua::isMap(int stack_index) const -> bool
{
    return lua_istable(m_lua.get(), stack_index) != 0;
}
auto GluaLua::setGlobalFromStack(const std::string& name, int stack_index)
    -> void
{
    auto absolute_value_index = absoluteIndex(stack_index);
    lua_getglobal(m_lua.get(), "__libglua__env__");
    lua_pushlstring(m_lua.get(), name.data(), name.size());
    lua_pushvalue(
        m_lua.get(),
        absolute_value_index); // get value from original stack back into position

    lua_settable(m_lua.get(), -3);

    lua_pop(m_lua.get(), 1); // env table is still on stack, pop
}
auto GluaLua::pushGlobal(const std::string& name) -> void
{
    lua_getglobal(m_lua.get(), "__libglua__env__");
    lua_pushlstring(m_lua.get(), name.data(), name.size());

    lua_gettable(m_lua.get(), -2);

    lua_remove(m_lua.get(), -2); // remove sandbox env from stack
}
auto GluaLua::popOffStack(size_t count) -> void
{
    lua_pop(m_lua.get(), static_cast<int>(count));
}
auto GluaLua::getStackTop() -> int { return lua_gettop(m_lua.get()); }
auto GluaLua::callScriptFunctionImpl(const std::string& function_name,
    size_t arg_count) -> void
{
    pushValueOfGlobalOntoStack(function_name);

    if (!lua_isfunction(m_lua.get(), -1)) {
        throw exceptions::LuaException("Attempted to call lua script function " + function_name + " which was not a function");
    }

    // arg_count arguments were already on the stack, but lua requires the
    // function before the arguments
    if (arg_count > 0) {
        // need to move the function back arg_count layers on the stack
        lua_insert(m_lua.get(), -1 - static_cast<int>(arg_count));
    }

    lua_getglobal(m_lua.get(), "__libglua__env__");
    lua_setfenv(m_lua.get(), -2);
    if (lua_pcall(m_lua.get(), static_cast<int>(arg_count), LUA_MULTRET, 0) != 0) {
        throw exceptions::LuaException("Failed to call lua script function [" + function_name + "]: " + lua_tostring(m_lua.get(), -1));
    }
}
auto GluaLua::registerClassImpl(
    const std::string& class_name,
    std::unordered_map<std::string, std::unique_ptr<ICallable>>
        method_callables) -> void
{
    luaL_newmetatable(m_lua.get(), class_name.data());

    lua_pushstring(m_lua.get(), "__gc");
    lua_pushcfunction(m_lua.get(), &destruct_managed_type);
    lua_settable(m_lua.get(), -3);

    auto pos_pair = m_method_registry.emplace(class_name, std::move(method_callables));
    auto& our_registry = pos_pair.first->second;

    lua_pushstring(m_lua.get(), "__index");
    lua_pushvalue(m_lua.get(), -2);
    lua_settable(m_lua.get(), -3);

    for (auto& method_pair : our_registry) {
        lua_pushstring(m_lua.get(), method_pair.first.data());
        auto* callable_ptr = method_pair.second.get();
        lua_pushlightuserdata(m_lua.get(), callable_ptr);
        lua_pushcclosure(m_lua.get(), call_callable_from_lua, 1);

        lua_settable(m_lua.get(), -3);
    }

    // pop the new metatable off the stack
    lua_pop(m_lua.get(), 1);
}
auto GluaLua::registerMethodImpl(const std::string& class_name,
    const std::string& method_name,
    Callable method) -> void
{
    luaL_getmetatable(m_lua.get(), class_name.data());

    auto pos_pair = m_method_registry[class_name].emplace(
        method_name, std::move(method).AcquireCallable());

    if (pos_pair.second) {
        lua_pushlstring(m_lua.get(), method_name.data(), method_name.size());
        auto* callable_ptr = pos_pair.first->second.get();
        lua_pushlightuserdata(m_lua.get(), callable_ptr);
        lua_pushcclosure(m_lua.get(), call_callable_from_lua, 1);
    } else {
        throw exceptions::LuaException(
            "Tried to register method with already registered name [" + method_name + "]");
    }

    lua_settable(m_lua.get(), -3);
}
auto GluaLua::transformObjectIndex(size_t index) -> size_t
{
    return index + 1; // lua is one based
}
auto GluaLua::transformFunctionParameterIndex(size_t index) -> size_t
{
    return index + 1; // function parameter indices start at 1
}
auto GluaLua::runScript(std::string_view script_data) -> void
{
    auto code = luaL_loadbuffer(m_lua.get(), script_data.data(),
        script_data.size(), "libglua");

    if (code == 0) {
        lua_getglobal(m_lua.get(), "__libglua__env__");
        lua_setfenv(m_lua.get(), -2);
        code = lua_pcall(m_lua.get(), 0, LUA_MULTRET, 0);

        if (code != 0) {
            std::string message { "Failed to call script: " };
            message.append(lua_tostring(m_lua.get(), -1));
            throw exceptions::LuaException(std::move(message));
        }
    } else {
        throw exceptions::LuaException(std::string { "Failed to load script" }.append(
            lua_tostring(m_lua.get(), -1)));
    }
}
auto GluaLua::GluaLua::pushValueOfGlobalOntoStack(
    const std::string& global_name) -> void
{
    lua_getglobal(m_lua.get(), "__libglua__env__");
    lua_pushlstring(m_lua.get(), global_name.data(), global_name.size());

    lua_gettable(m_lua.get(), -2);

    lua_remove(m_lua.get(), -2); // remove sandbox env from stack
}
auto GluaLua::setValueOfGlobalFromTopOfStack(const std::string& global_name)
    -> void
{
    lua_getglobal(m_lua.get(), "__libglua__env__");
    lua_pushlstring(m_lua.get(), global_name.data(), global_name.size());
    lua_pushvalue(m_lua.get(),
        -3); // get value from original stack back into position

    lua_settable(
        m_lua.get(),
        -3); // now stack has original value, table, poth should be popped

    lua_pop(m_lua.get(), 2);
}

auto GluaLua::absoluteIndex(int index) const -> int
{
    if (index > 0 || index < LUA_REGISTRYINDEX) {
        return index;
    }

    return lua_gettop(m_lua.get()) + index + 1;
}

auto call_callable_from_lua(lua_State* state) -> int
{
    auto* callable_ptr = static_cast<ICallable*>(lua_touserdata(state, lua_upvalueindex(1)));

    callable_ptr->Call();

    // C++ only 1 return possible, so either nothing was pushed or 1 item was
    return callable_ptr->HasReturn() ? 1 : 0;
}

auto destruct_managed_type(lua_State* state) -> int
{
    auto** managed_type_ptr = static_cast<IManagedTypeStorage**>(lua_touserdata(state, 1));

    std::unique_ptr<IManagedTypeStorage> reacquired_memory { *managed_type_ptr };

    return 0;
}

} // namespace kdk::glua
