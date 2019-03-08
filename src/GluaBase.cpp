#include "glua/GluaBase.h"

namespace kdk::glua
{
auto GluaBase::PushChild(int parent_index, size_t child_index) -> StackPosition
{
    getArrayValue(child_index, parent_index);

    return StackPosition{this, getStackTop()};
}

auto GluaBase::SafePushChild(int parent_index, size_t child_index) -> StackPosition
{
    getArrayValue(child_index, parent_index);

    auto result_index = getStackTop();

    if (isNull(result_index))
    {
        throw std::out_of_range("GluaBase::SafePushChild with unset index");
    }

    return StackPosition{this, result_index};
}

auto GluaBase::PushChild(int parent_index, const std::string& child_key) -> StackPosition
{
    getMapValue(child_key, parent_index);

    return StackPosition{this, getStackTop()};
}

auto GluaBase::SafePushChild(int parent_index, const std::string& child_key) -> StackPosition
{
    getMapValue(child_key, parent_index);

    auto result_index = getStackTop();

    if (isNull(result_index))
    {
        throw std::out_of_range("GluaBase::SafePushChild with unset key");
    }

    return StackPosition{this, result_index};
}

auto GluaBase::PushGlobal(const std::string& name) -> StackPosition
{
    pushGlobal(name);

    return StackPosition{this, getStackTop()};
}
} // namespace kdk::glua
