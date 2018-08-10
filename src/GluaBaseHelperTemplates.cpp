#include "glua/GluaBase.h"

namespace kdk::glua
{
auto GluaResolver<bool>::get(GluaBase* glua, int stack_index) -> bool
{
    return glua->getBool(stack_index);
}
auto GluaResolver<bool>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isBool(stack_index);
}

auto GluaResolver<int8_t>::get(GluaBase* glua, int stack_index) -> int8_t
{
    return glua->getInt8(stack_index);
}
auto GluaResolver<int8_t>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isInt8(stack_index);
}

auto GluaResolver<int16_t>::get(GluaBase* glua, int stack_index) -> int16_t
{
    return glua->getInt16(stack_index);
}
auto GluaResolver<int16_t>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isInt16(stack_index);
}

auto GluaResolver<int32_t>::get(GluaBase* glua, int stack_index) -> int32_t
{
    return glua->getInt32(stack_index);
}
auto GluaResolver<int32_t>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isInt32(stack_index);
}

auto GluaResolver<int64_t>::get(GluaBase* glua, int stack_index) -> int64_t
{
    return glua->getInt64(stack_index);
}
auto GluaResolver<int64_t>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isInt64(stack_index);
}

auto GluaResolver<uint8_t>::get(GluaBase* glua, int stack_index) -> uint8_t
{
    return glua->getUInt8(stack_index);
}
auto GluaResolver<uint8_t>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isUInt8(stack_index);
}

auto GluaResolver<uint16_t>::get(GluaBase* glua, int stack_index) -> uint16_t
{
    return glua->getUInt16(stack_index);
}
auto GluaResolver<uint16_t>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isUInt16(stack_index);
}

auto GluaResolver<uint32_t>::get(GluaBase* glua, int stack_index) -> uint32_t
{
    return glua->getUInt32(stack_index);
}
auto GluaResolver<uint32_t>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isUInt32(stack_index);
}

auto GluaResolver<uint64_t>::get(GluaBase* glua, int stack_index) -> uint64_t
{
    return glua->getUInt64(stack_index);
}
auto GluaResolver<uint64_t>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isUInt64(stack_index);
}

auto GluaResolver<float>::get(GluaBase* glua, int stack_index) -> float
{
    return glua->getFloat(stack_index);
}
auto GluaResolver<float>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isFloat(stack_index);
}

auto GluaResolver<double>::get(GluaBase* glua, int stack_index) -> double
{
    return glua->getDouble(stack_index);
}
auto GluaResolver<double>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isDouble(stack_index);
}

auto GluaResolver<const char*>::get(GluaBase* glua, int stack_index) -> const char*
{
    return glua->getCharPointer(stack_index);
}
auto GluaResolver<const char*>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isCharPointer(stack_index);
}

auto GluaResolver<std::string_view>::get(GluaBase* glua, int stack_index) -> std::string_view
{
    return glua->getStringView(stack_index);
}
auto GluaResolver<std::string_view>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isStringView(stack_index);
}

auto GluaResolver<std::string>::get(GluaBase* glua, int stack_index) -> std::string
{
    return glua->getString(stack_index);
}
auto GluaResolver<std::string>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isString(stack_index);
}
} // namespace kdk::glua