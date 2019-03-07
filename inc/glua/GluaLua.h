#pragma once

#include "glua/GluaBase.h"

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

namespace kdk::glua
{
struct LuaStateDeleter
{
    auto operator()(lua_State* state) -> void;
};

class GluaLua : public GluaBase
{
public:
    /**
     * @brief Constructs a new GluaLua object
     *
     * @param output_stream stream to which lua 'print' output will be redirected
     * @param start_sandboxed true if the starting environment should be sandboxed
     *                        to protect from dangerous functions like file i/o, etc
     */
    explicit GluaLua(std::ostream& output_stream, bool start_sandboxed = true);

    GluaLua(const GluaLua&)     = delete;
    GluaLua(GluaLua&&) noexcept = default;

    auto operator=(const GluaLua&) -> GluaLua& = delete;
    auto operator=(GluaLua&&) noexcept -> GluaLua& = default;

    /**
     * @brief Retrieves a GluaLua instance from a lua_State object, if that
     * lua_State was created and managed by a GluaLua instance
     *
     * @param lua the lua state to retrieve the instance from
     * @return a reference to the GluaLua instance for the given state
     */
    static auto GetInstanceFromState(lua_State* lua) -> GluaLua&;

    /** GluaBase public interface, implemented by language specific derivations **/

    /**
     * @copydoc GluaBase::ResetEnvironment(bool)
     */
    auto ResetEnvironment(bool sandboxed = true) -> void override;
    /**
     * @copydoc GluaBase::RegisterCallable(const std::string&, Callable)
     */
    auto RegisterCallable(const std::string& name, Callable callable) -> void override;
    /**
     * @copydoc GluaBase::RunScript(std::string_view)
     */
    auto RunScript(std::string_view script_data) -> void override;
    /**
     * @copydoc GluaBase::RunFile(std::string_view)
     */
    auto RunFile(std::string_view file_name) -> void override;
    /*****************************************************************************/

    /**
     * @brief defaulted destructor override
     */
    ~GluaLua() override = default;

protected:
    /** GluaBase protected interface, implemented by language specific derivations **/
    auto push(std::nullopt_t /*unused*/) -> void override;
    auto push(bool value) -> void override;
    auto push(int8_t value) -> void override;
    auto push(int16_t value) -> void override;
    auto push(int32_t value) -> void override;
    auto push(int64_t value) -> void override;
    auto push(uint8_t value) -> void override;
    auto push(uint16_t value) -> void override;
    auto push(uint32_t value) -> void override;
    auto push(uint64_t value) -> void override;
    auto push(float value) -> void override;
    auto push(double value) -> void override;
    auto push(const char* value) -> void override;
    auto push(std::string_view value) -> void override;
    auto push(std::string value) -> void override;
    auto pushArray(size_t size_hint) -> void override;
    auto pushStartMap(size_t size_hint) -> void override;
    auto arraySetFromStack() -> void override;
    auto mapSetFromStack() -> void override;
    auto pushUserType(const std::string& unique_type_name, std::unique_ptr<IManagedTypeStorage> user_storage)
        -> void override;
    auto getBool(int stack_index) const -> bool override;
    auto getInt8(int stack_index) const -> int8_t override;
    auto getInt16(int stack_index) const -> int16_t override;
    auto getInt32(int stack_index) const -> int32_t override;
    auto getInt64(int stack_index) const -> int64_t override;
    auto getUInt8(int stack_index) const -> uint8_t override;
    auto getUInt16(int stack_index) const -> uint16_t override;
    auto getUInt32(int stack_index) const -> uint32_t override;
    auto getUInt64(int stack_index) const -> uint64_t override;
    auto getFloat(int stack_index) const -> float override;
    auto getDouble(int stack_index) const -> double override;
    auto getCharPointer(int stack_index) const -> const char* override;
    auto getStringView(int stack_index) const -> std::string_view override;
    auto getString(int stack_index) const -> std::string override;
    auto getArraySize(int stack_index) const -> size_t override;
    auto getArrayValue(size_t index_into_array, int stack_index_of_array) const -> void override;
    auto getMapKeys(int stack_index) const -> std::vector<std::string> override;
    auto getMapValue(const std::string& key, int stack_index_of_map) const -> void override;
    auto getUserType(const std::string& unique_type_name, int stack_index) const -> IManagedTypeStorage* override;
    auto isUserType(const std::string& unique_type_name, int stack_index) const -> bool override;
    auto isNull(int stack_index) const -> bool override;
    auto isBool(int stack_index) const -> bool override;
    auto isInt8(int stack_index) const -> bool override;
    auto isInt16(int stack_index) const -> bool override;
    auto isInt32(int stack_index) const -> bool override;
    auto isInt64(int stack_index) const -> bool override;
    auto isUInt8(int stack_index) const -> bool override;
    auto isUInt16(int stack_index) const -> bool override;
    auto isUInt32(int stack_index) const -> bool override;
    auto isUInt64(int stack_index) const -> bool override;
    auto isFloat(int stack_index) const -> bool override;
    auto isDouble(int stack_index) const -> bool override;
    auto isCharPointer(int stack_index) const -> bool override;
    auto isStringView(int stack_index) const -> bool override;
    auto isString(int stack_index) const -> bool override;
    auto isArray(int stack_index) const -> bool override;
    auto isMap(int stack_index) const -> bool override;
    auto setGlobalFromStack(const std::string& name, int stack_index) -> void override;
    auto pushGlobal(const std::string& name) -> void override;
    auto popOffStack(size_t count) -> void override;
    auto getStackTop() -> int override;
    auto callScriptFunctionImpl(const std::string& function_name, size_t arg_count = 0) -> void override;
    auto registerClassImpl(
        const std::string&                                          class_name,
        std::unordered_map<std::string, std::unique_ptr<ICallable>> method_callables) -> void override;
    auto registerMethodImpl(const std::string& class_name, const std::string& method_name, Callable method)
        -> void override;
    auto transformObjectIndex(size_t index) -> size_t override;
    auto transformFunctionParameterIndex(size_t index) -> size_t override;
    /********************************************************************************/

private:
    auto pushValueOfGlobalOntoStack(const std::string& global_name) -> void;
    auto setValueOfGlobalFromTopOfStack(const std::string& global_name) -> void;
    auto absoluteIndex(int index) const -> int;

    std::unique_ptr<lua_State, LuaStateDeleter> m_lua;

    std::unordered_map<std::string, std::unique_ptr<ICallable>>                                  m_registry;
    std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<ICallable>>> m_method_registry;
    std::unordered_map<std::type_index, std::string> m_class_to_metatable_name;

    std::ostream& m_output_stream;

    std::optional<size_t>      m_current_array_index;
    std::optional<std::string> m_current_map_key;
};

auto call_callable_from_lua(lua_State* state) -> int;
auto destruct_managed_type(lua_State* state) -> int;

} // namespace kdk::glua
