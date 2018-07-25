#include "Glua.h"

#include "FileUtil.h"

namespace kdk::glua
{
auto Glua::Create(std::ostream& output_stream) -> Ptr
{
    return Ptr{new Glua(output_stream)};
}

auto Glua::Push(bool value) -> void
{
    lua_pushboolean(m_lua, value ? 1 : 0);
}

auto Glua::Push(int8_t value) -> void
{
    lua_pushinteger(m_lua, value);
}

auto Glua::Push(int16_t value) -> void
{
    lua_pushinteger(m_lua, value);
}

auto Glua::Push(int32_t value) -> void
{
    lua_pushinteger(m_lua, value);
}

auto Glua::Push(int64_t value) -> void
{
    lua_pushinteger(m_lua, value);
}

auto Glua::Push(uint8_t value) -> void
{
    lua_pushinteger(m_lua, static_cast<long long>(value));
}

auto Glua::Push(uint16_t value) -> void
{
    lua_pushinteger(m_lua, static_cast<long long>(value));
}

auto Glua::Push(uint32_t value) -> void
{
    lua_pushinteger(m_lua, static_cast<long long>(value));
}

auto Glua::Push(uint64_t value) -> void
{
    lua_pushinteger(m_lua, static_cast<long long>(value));
}

auto Glua::Push(float value) -> void
{
    lua_pushnumber(m_lua, static_cast<double>(value));
}
auto Glua::Push(double value) -> void
{
    lua_pushnumber(m_lua, value);
}
auto Glua::Push(const char* value) -> void
{
    lua_pushstring(m_lua, value);
}
auto Glua::Push(std::string_view value) -> void
{
    lua_pushlstring(m_lua, value.data(), value.size());
}
auto Glua::Push(std::string value) -> void
{
    lua_pushlstring(m_lua, value.data(), value.size());
}

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> bool
{
    auto ret_val = static_cast<bool>(lua_toboolean(lua, stack_index));

    return ret_val;
}

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> int8_t
{
    auto ret_val = static_cast<int8_t>(lua_tointeger(lua, stack_index));

    return ret_val;
}

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> int16_t
{
    auto ret_val = static_cast<int16_t>(lua_tointeger(lua, stack_index));

    return ret_val;
}

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> int32_t
{
    auto ret_val = static_cast<int32_t>(lua_tointeger(lua, stack_index));

    return ret_val;
}

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> int64_t
{
    auto ret_val = static_cast<int64_t>(lua_tointeger(lua, stack_index));

    return ret_val;
}

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> uint8_t
{
    auto ret_val = static_cast<uint8_t>(lua_tointeger(lua, stack_index));

    return ret_val;
}

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> uint16_t
{
    auto ret_val = static_cast<uint16_t>(lua_tointeger(lua, stack_index));

    return ret_val;
}

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> uint32_t
{
    auto ret_val = static_cast<uint32_t>(lua_tointeger(lua, stack_index));

    return ret_val;
}

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> uint64_t
{
    auto ret_val = static_cast<uint64_t>(lua_tointeger(lua, stack_index));

    return ret_val;
}

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> float
{
    auto ret_val = static_cast<float>(lua_tonumber(lua, stack_index));

    return ret_val;
}

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> double
{
    auto ret_val = static_cast<double>(lua_tonumber(lua, stack_index));

    return ret_val;
}

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> std::string
{
    size_t      length;
    const auto* c_str = lua_tolstring(lua, stack_index, &length);

    std::string ret_val{c_str, length};

    lua_pop(lua, 1);

    return ret_val;
}

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> std::string_view
{
    size_t      length;
    const auto* c_str = lua_tolstring(lua, stack_index, &length);

    std::string_view ret_val{c_str, length};

    return ret_val;
}

auto Glua::RunScript(std::string_view script_data) -> void
{
    auto code = luaL_loadbuffer(m_lua, script_data.data(), script_data.size(), "libglua");

    if (!code)
    {
        lua_getglobal(m_lua, "__libglua__env__");
        lua_setfenv(m_lua, -2);
        code = lua_pcall(m_lua, 0, 0, 0);

        if (code)
        {
            std::string message{"Failed to call script: "};
            message.append(lua_tostring(m_lua, -1));
            throw exceptions::LuaException(std::move(message));
        }
    }
    else
    {
        throw exceptions::LuaException("Failed to load script");
    }
}

auto Glua::RunFile(std::string_view file_name) -> void
{
    auto file_str = file_util::read_all(file_name);

    RunScript(file_str);
}
auto Glua::CallScriptFunction(const std::string& function_name) -> void
{
    get_global_onto_stack(function_name.data());

    if (!lua_isfunction(m_lua, -1))
    {
        throw exceptions::LuaException(
            "Attempted to call lua script function " + function_name + " which was not a function");
    }

    lua_getglobal(m_lua, "__libglua__env__");
    lua_setfenv(m_lua, -2);
    if (lua_pcall(m_lua, 0, LUA_MULTRET, 0))
    {
        throw exceptions::LuaException(
            "Failed to call lua script function [" + function_name + "]: " + lua_tostring(m_lua, -1));
    }
}

auto Glua::RegisterCallable(std::string_view name, Callable callable) -> void
{
    auto insert_pair = m_registry.emplace(std::string{name}, std::move(callable).AcquireCallable());

    if (insert_pair.second)
    {
        lua_getglobal(m_lua, "__libglua__sandbox__");

        lua_pushlstring(m_lua, name.data(), name.size());

        auto* callable_ptr = insert_pair.first->second.get();
        lua_pushlightuserdata(m_lua, callable_ptr);
        lua_pushcclosure(m_lua, call_callable_from_lua, 1);

        lua_settable(m_lua, -3);

        lua_pop(m_lua, 1);
    }
    else
    {
        throw exceptions::LuaException("Registered a callable with an already used name");
    }
}

auto Glua::GetLuaState() -> lua_State*
{
    return m_lua;
}

auto Glua::GetLuaState() const -> const lua_State*
{
    return m_lua;
}

auto Glua::ResetEnvironment() -> void
{
    lua_getglobal(m_lua, "__libglua__sandbox__");

    lua_newtable(m_lua);               // env
    lua_pushvalue(m_lua, -1);          // env -> env
    lua_pushliteral(m_lua, "__index"); // env -> env -> __index
    lua_pushvalue(m_lua, -4);          // push __libglua__sandbox__, env -> env -> __index -> sandbox
    lua_settable(m_lua, -3);           // env -> env
    lua_setmetatable(m_lua, -2);       // env

    lua_setglobal(m_lua, "__libglua__env__");
}

Glua::~Glua()
{
    if (m_lua)
    {
        lua_close(m_lua);
    }
}

static auto glua_capture_print(lua_State* lua) -> int
{
    auto  arg_count = lua_gettop(lua);
    auto& ostream   = *static_cast<std::ostream*>(lua_touserdata(lua, lua_upvalueindex(1)));

    for (auto i = 1; i <= arg_count; ++i)
    {
        ostream << lua_tostring(lua, i);
    }

    ostream << std::endl; // print ends with newline

    return 0;
}

static auto glua_set_environment_to_stack(lua_State* lua, const std::vector<std::string>& environment) -> void
{
    // table we're setting is currently top  (-1) on stack

    for (const auto& str : environment)
    {
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
glua_set_sub_environment_to_stack(lua_State* lua, std::string_view parent_table, const std::vector<std::string>& environment)
    -> void
{
    // table we're setting is currently top  (-1) on stack

    lua_pushlstring(lua, parent_table.data(), parent_table.size());
    lua_createtable(lua, 0, static_cast<int>(environment.size()));

    for (const auto& str : environment)
    {
        lua_pushlstring(lua, str.data(), str.length());

        lua_getglobal(lua, parent_table.data());
        lua_pushlstring(lua, str.data(), str.length());
        lua_gettable(lua, -2);

        lua_remove(lua, -2); // remove parent table from stack

        lua_settable(lua, -3);
    }

    lua_settable(lua, -3);
}

Glua::Glua(std::ostream& output_stream) : m_lua(luaL_newstate()), m_output_stream(output_stream)
{
    luaL_openlibs(m_lua);

    luaL_Reg print_override_lib[] = {{"print", glua_capture_print}, {nullptr, nullptr}};

    // create sandbox environment
    lua_newtable(m_lua);

    std::vector<std::string> sandbox_environment{"assert",
                                                 "error",
                                                 "ipairs",
                                                 "next",
                                                 "pairs",
                                                 "pcall",
                                                 "print"
                                                 "select",
                                                 "tonumber",
                                                 "tostring",
                                                 "type",
                                                 "unpack",
                                                 "_VERSION",
                                                 "xpcall"};
    std::vector<std::string> sandbox_coroutine_environment{"create", "resume", "running", "status", "wrap", "yield"};
    std::vector<std::string> sandbox_io_environment{"read", "write", "flush", "type"};
    std::vector<std::string> sandbox_string_environment{
        "byte", "char", "dump", "find", "format", "gmatch", "gsub", "len", "lower", "match", "rep", "reverse", "sub", "upper"};
    std::vector<std::string> sandbox_table_environment{"insert", "maxn", "remove", "sort"};
    std::vector<std::string> sandbox_math_environment{
        "abs",   "acos", "asin",  "atan",   "atan2", "ceil", "cos",   "cosh", "deg", "exp",
        "floor", "fmod", "frexp", "huge",   "ldexp", "log",  "log10", "max",  "min", "modf",
        "pi",    "pow",  "rad",   "random", "sin",   "sinh", "sqrt",  "tan",  "tanh"};
    std::vector<std::string> sandbox_os_environment{"clock", "difftime", "time"};

    glua_set_environment_to_stack(m_lua, sandbox_environment);
    glua_set_sub_environment_to_stack(m_lua, "coroutine", sandbox_coroutine_environment);
    glua_set_sub_environment_to_stack(m_lua, "io", sandbox_io_environment);
    glua_set_sub_environment_to_stack(m_lua, "string", sandbox_string_environment);
    glua_set_sub_environment_to_stack(m_lua, "table", sandbox_table_environment);
    glua_set_sub_environment_to_stack(m_lua, "math", sandbox_math_environment);
    glua_set_sub_environment_to_stack(m_lua, "os", sandbox_os_environment);

    lua_pushlightuserdata(m_lua, &m_output_stream);
    luaL_setfuncs(m_lua, print_override_lib, 1);

    lua_setglobal(m_lua, "__libglua__sandbox__");

    // now that sandbox is set up, reset environment
    ResetEnvironment();

    lua_pushlightuserdata(m_lua, this);
    lua_setglobal(m_lua, "LuaClass");
}

auto Glua::get_global_onto_stack(const std::string& global_name) -> void
{
    lua_getglobal(m_lua, "__libglua__env__");
    lua_pushlstring(m_lua, global_name.data(), global_name.size());

    lua_gettable(m_lua, -2);

    lua_remove(m_lua, -2); // remove sandbox env from stack
}

auto Glua::set_global_from_stack(const std::string& global_name) -> void
{
    lua_getglobal(m_lua, "__libglua__env__");
    lua_pushlstring(m_lua, global_name.data(), global_name.size());
    lua_pushvalue(m_lua, -3); // get value from original stack back into position

    lua_settable(m_lua, -3); // now stack has original value, table, poth should be popped

    lua_pop(m_lua, 2);
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
    IManagedTypeStorage* managed_type_ptr = static_cast<IManagedTypeStorage*>(lua_touserdata(state, 1));
    managed_type_ptr->~IManagedTypeStorage();

    return 0;
}

auto remove_all_whitespace(std::string_view input) -> std::string
{
    std::string output;
    output.reserve(input.size());
    for (auto c : input)
    {
        if (!std::isspace(c))
        {
            output.append({c});
        }
    }

    return output;
}

auto split(std::string_view input, std::string_view token) -> std::vector<std::string_view>
{
    std::vector<std::string_view> output;

    auto pos = input.find(token);

    while (!input.empty() && pos != std::string_view::npos)
    {
        auto substr = input.substr(0, pos);
        input       = input.substr(pos + 1); // skip comma

        pos = input.find(token);
        output.emplace_back(std::move(substr));
    }

    if (!input.empty())
    {
        output.emplace_back(std::move(input));
    }

    return output;
}

} // namespace kdk::glua
