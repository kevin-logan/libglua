#include "glua/GluaBase.h"

namespace kdk::glua
{
auto GluaBase::PushChild(int parent_index, size_t child_index) -> int
{
    getArrayValue(child_index, parent_index);

    return getStackTop();
}

auto GluaBase::SafePushChild(int parent_index, size_t child_index) -> int
{
    getArrayValue(child_index, parent_index);

    auto result_index = getStackTop();

    if (isNull(result_index))
    {
        throw std::out_of_range("GluaBase::SafePushChild with unset index");
    }

    return result_index;
}

auto GluaBase::PushChild(int parent_index, const std::string& child_key) -> int
{
    getMapValue(child_key, parent_index);

    return getStackTop();
}

auto GluaBase::SafePushChild(int parent_index, const std::string& child_key) -> int
{
    getMapValue(child_key, parent_index);

    auto result_index = getStackTop();

    if (isNull(result_index))
    {
        throw std::out_of_range("GluaBase::SafePushChild with unset key");
    }

    return result_index;
}

auto GluaBase::PushGlobal(const std::string& name) -> int
{
    pushGlobal(name);

    return getStackTop();
}

} // namespace kdk::glua
