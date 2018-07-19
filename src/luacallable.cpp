#include "luacallable.h"

#include "file_util.h"

namespace kdk::lua
{
auto Lua::Create() -> std::shared_ptr<Lua>
{
    return std::shared_ptr<Lua>{new Lua()};
}

auto Lua::Push(bool value) -> void
{
    lua_pushboolean(m_lua, value ? 1 : 0);
}

auto Lua::Push(int8_t value) -> void
{
    lua_pushinteger(m_lua, value);
}

auto Lua::Push(int16_t value) -> void
{
    lua_pushinteger(m_lua, value);
}

auto Lua::Push(int32_t value) -> void
{
    lua_pushinteger(m_lua, value);
}

auto Lua::Push(int64_t value) -> void
{
    lua_pushinteger(m_lua, value);
}

auto Lua::Push(uint8_t value) -> void
{
    lua_pushinteger(m_lua, static_cast<long long>(value));
}

auto Lua::Push(uint16_t value) -> void
{
    lua_pushinteger(m_lua, static_cast<long long>(value));
}

auto Lua::Push(uint32_t value) -> void
{
    lua_pushinteger(m_lua, static_cast<long long>(value));
}

auto Lua::Push(uint64_t value) -> void
{
    lua_pushinteger(m_lua, static_cast<long long>(value));
}

auto Lua::Push(float value) -> void
{
    lua_pushnumber(m_lua, static_cast<double>(value));
}
auto Lua::Push(double value) -> void
{
    lua_pushnumber(m_lua, value);
}
auto Lua::Push(const char* value) -> void
{
    lua_pushstring(m_lua, value);
}
auto Lua::Push(std::string_view value) -> void
{
    lua_pushlstring(m_lua, value.data(), value.size());
}
auto Lua::Push(std::string value) -> void
{
    lua_pushlstring(m_lua, value.data(), value.size());
}

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> bool
{
    auto ret_val = static_cast<bool>(lua_toboolean(lua, stack_index));

    return ret_val;
}

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> int8_t
{
    auto ret_val = static_cast<int8_t>(lua_tointeger(lua, stack_index));

    return ret_val;
}

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> int16_t
{
    auto ret_val = static_cast<int16_t>(lua_tointeger(lua, stack_index));

    return ret_val;
}

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> int32_t
{
    auto ret_val = static_cast<int32_t>(lua_tointeger(lua, stack_index));

    return ret_val;
}

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> int64_t
{
    auto ret_val = static_cast<int64_t>(lua_tointeger(lua, stack_index));

    return ret_val;
}

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> uint8_t
{
    auto ret_val = static_cast<uint8_t>(lua_tointeger(lua, stack_index));

    return ret_val;
}

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> uint16_t
{
    auto ret_val = static_cast<uint16_t>(lua_tointeger(lua, stack_index));

    return ret_val;
}

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> uint32_t
{
    auto ret_val = static_cast<uint32_t>(lua_tointeger(lua, stack_index));

    return ret_val;
}

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> uint64_t
{
    auto ret_val = static_cast<uint64_t>(lua_tointeger(lua, stack_index));

    return ret_val;
}

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> float
{
    auto ret_val = static_cast<float>(lua_tonumber(lua, stack_index));

    return ret_val;
}

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> double
{
    auto ret_val = static_cast<double>(lua_tonumber(lua, stack_index));

    return ret_val;
}

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> std::string
{
    size_t      length;
    const auto* c_str = lua_tolstring(lua, stack_index, &length);

    std::string ret_val{c_str, length};

    lua_pop(lua, 1);

    return ret_val;
}

template<>
auto Lua::get(lua_State* lua, int32_t stack_index) -> std::string_view
{
    size_t      length;
    const auto* c_str = lua_tolstring(lua, stack_index, &length);

    std::string_view ret_val{c_str, length};

    return ret_val;
}

auto Lua::RunScript(std::string_view script_data) -> void
{
    auto code = luaL_loadbuffer(m_lua, script_data.data(), script_data.size(), "lualib");

    if (!code)
    {
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

auto Lua::RunFile(std::string_view file_name) -> void
{
    auto file_str = file_util::read_all(file_name);

    RunScript(file_str);
}
auto Lua::CallScriptFunction(const std::string& function_name) -> void
{
    lua_getglobal(m_lua, function_name.data());

    if (!lua_isfunction(m_lua, -1))
    {
        throw exceptions::LuaException(
            "Attempted to call lua script function" + function_name + " which was not a function");
    }

    if (lua_pcall(m_lua, 0, LUA_MULTRET, 0))
    {
        throw exceptions::LuaException("Failed to call lua script function" + function_name);
    }
}

auto Lua::RegisterCallable(std::string_view name, Callable callable) -> void
{
    auto insert_pair = m_registry.emplace(std::string{name}, std::move(callable).AcquireCallable());

    if (insert_pair.second)
    {
        auto* callable_ptr = insert_pair.first->second.get();
        lua_pushlightuserdata(m_lua, callable_ptr);
        lua_pushcclosure(m_lua, call_callable_from_lua, 1);
        lua_setglobal(m_lua, name.data());
    }
    else
    {
        throw exceptions::LuaException("Registered a callable with an already used name");
    }
}

auto Lua::GetLuaState() -> lua_State*
{
    return m_lua;
}

auto Lua::GetLuaState() const -> const lua_State*
{
    return m_lua;
}

auto Lua::GetNewOutput() -> std::string
{
    auto ret_val = m_output_stream.str();
    m_output_stream.str("");
    m_output_stream.clear();

    return ret_val;
}

Lua::~Lua()
{
    if (m_lua)
    {
        lua_close(m_lua);
    }
}

static auto lua_capture_print(lua_State* lua) -> int
{
    auto  arg_count = lua_gettop(lua);
    auto& sstream   = *static_cast<std::stringstream*>(lua_touserdata(lua, lua_upvalueindex(1)));

    for (auto i = 1; i <= arg_count; ++i)
    {
        sstream << lua_tostring(lua, i);
    }

    sstream << "\n"; // print ends with newline

    return 0;
}

Lua::Lua() : m_lua(luaL_newstate())
{
    luaL_openlibs(m_lua);

    luaL_Reg print_override_lib[] = {{"print", lua_capture_print}, {nullptr, nullptr}};

    lua_getglobal(m_lua, "_G");
    lua_pushlightuserdata(m_lua, &m_output_stream);
    luaL_setfuncs(m_lua, print_override_lib, 1);
    lua_pop(m_lua, 1);

    lua_pushlightuserdata(m_lua, this);
    lua_setglobal(m_lua, "LuaClass");
}

auto call_callable_from_lua(lua_State* state) -> int
{
    auto* callable_ptr = static_cast<ICallable*>(lua_touserdata(state, lua_upvalueindex(1)));

    callable_ptr->Call();

    // C++ only 1 return possible, so either nothing was pushed or 1 item was
    return callable_ptr->HasReturn() ? 1 : 0;
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

} // namespace kdk::lua
