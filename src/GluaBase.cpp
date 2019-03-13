#include "glua/GluaBase.h"

#include "glua/FileUtil.h"

namespace kdk::glua {
auto GluaBase::PushChild(int parent_index, size_t child_index)
    -> StackPosition
{
    getArrayValue(transformObjectIndex(child_index), parent_index);

    return StackPosition { this, getStackTop() };
}

auto GluaBase::SafePushChild(int parent_index, size_t child_index)
    -> StackPosition
{
    getArrayValue(transformObjectIndex(child_index), parent_index);

    auto result_index = getStackTop();

    if (isNull(result_index)) {
        throw std::out_of_range("GluaBase::SafePushChild with unset index");
    }

    return StackPosition { this, result_index };
}

auto GluaBase::PushChild(int parent_index, const std::string& child_key)
    -> StackPosition
{
    getMapValue(child_key, parent_index);

    return StackPosition { this, getStackTop() };
}

auto GluaBase::SafePushChild(int parent_index, const std::string& child_key)
    -> StackPosition
{
    getMapValue(child_key, parent_index);

    auto result_index = getStackTop();

    if (isNull(result_index)) {
        throw std::out_of_range("GluaBase::SafePushChild with unset key");
    }

    return StackPosition { this, result_index };
}

auto GluaBase::IsArray(int stack_index) -> bool { return isArray(stack_index); }

auto GluaBase::IsMap(int stack_index) -> bool { return isMap(stack_index); }

auto GluaBase::GetArrayLength(int stack_index) -> size_t
{
    return getArraySize(stack_index);
}

auto GluaBase::PushGlobal(const std::string& name) -> StackPosition
{
    pushGlobal(name);

    return StackPosition { this, getStackTop() };
}

auto GluaBase::RunFile(std::string_view file_name)
    -> std::vector<StackPosition>
{
    auto file_str = file_util::read_all(file_name);

    return RunScript(file_str);
}

auto GluaBase::RunScript(std::string_view script_data)
    -> std::vector<StackPosition>
{
    auto previous_top = getStackTop();

    runScript(script_data);

    auto new_top = getStackTop();

    std::vector<StackPosition> results;

    if (new_top > previous_top) {
        auto value_count = static_cast<size_t>(new_top - previous_top);
        results.reserve(value_count);

        for (auto return_index = previous_top + 1; return_index <= new_top;
             ++return_index) {
            results.emplace_back(this, return_index);
        }
    }

    return results;
}

} // namespace kdk::glua
