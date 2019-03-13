#include "glua/GluaBase.h"

namespace kdk::glua {
auto GluaResolver<bool>::as(GluaBase* glua, int stack_index) -> bool
{
    return glua->getBool(stack_index);
}
auto GluaResolver<bool>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isBool(stack_index);
}
auto GluaResolver<bool>::push(GluaBase* glua, bool value) -> void
{
    glua->push(value);
}

auto GluaResolver<int8_t>::as(GluaBase* glua, int stack_index) -> int8_t
{
    return glua->getInt8(stack_index);
}
auto GluaResolver<int8_t>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isInt8(stack_index);
}
auto GluaResolver<int8_t>::push(GluaBase* glua, int8_t value) -> void
{
    glua->push(value);
}

auto GluaResolver<int16_t>::as(GluaBase* glua, int stack_index) -> int16_t
{
    return glua->getInt16(stack_index);
}
auto GluaResolver<int16_t>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isInt16(stack_index);
}
auto GluaResolver<int16_t>::push(GluaBase* glua, int16_t value) -> void
{
    glua->push(value);
}

auto GluaResolver<int32_t>::as(GluaBase* glua, int stack_index) -> int32_t
{
    return glua->getInt32(stack_index);
}
auto GluaResolver<int32_t>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isInt32(stack_index);
}
auto GluaResolver<int32_t>::push(GluaBase* glua, int32_t value) -> void
{
    glua->push(value);
}

auto GluaResolver<int64_t>::as(GluaBase* glua, int stack_index) -> int64_t
{
    return glua->getInt64(stack_index);
}
auto GluaResolver<int64_t>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isInt64(stack_index);
}
auto GluaResolver<int64_t>::push(GluaBase* glua, int64_t value) -> void
{
    glua->push(value);
}

auto GluaResolver<uint8_t>::as(GluaBase* glua, int stack_index) -> uint8_t
{
    return glua->getUInt8(stack_index);
}
auto GluaResolver<uint8_t>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isUInt8(stack_index);
}
auto GluaResolver<uint8_t>::push(GluaBase* glua, uint8_t value) -> void
{
    glua->push(value);
}

auto GluaResolver<uint16_t>::as(GluaBase* glua, int stack_index) -> uint16_t
{
    return glua->getUInt16(stack_index);
}
auto GluaResolver<uint16_t>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isUInt16(stack_index);
}
auto GluaResolver<uint16_t>::push(GluaBase* glua, uint16_t value) -> void
{
    glua->push(value);
}

auto GluaResolver<uint32_t>::as(GluaBase* glua, int stack_index) -> uint32_t
{
    return glua->getUInt32(stack_index);
}
auto GluaResolver<uint32_t>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isUInt32(stack_index);
}
auto GluaResolver<uint32_t>::push(GluaBase* glua, uint32_t value) -> void
{
    glua->push(value);
}

auto GluaResolver<uint64_t>::as(GluaBase* glua, int stack_index) -> uint64_t
{
    return glua->getUInt64(stack_index);
}
auto GluaResolver<uint64_t>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isUInt64(stack_index);
}
auto GluaResolver<uint64_t>::push(GluaBase* glua, uint64_t value) -> void
{
    glua->push(value);
}

auto GluaResolver<float>::as(GluaBase* glua, int stack_index) -> float
{
    return glua->getFloat(stack_index);
}
auto GluaResolver<float>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isFloat(stack_index);
}
auto GluaResolver<float>::push(GluaBase* glua, float value) -> void
{
    glua->push(value);
}

auto GluaResolver<double>::as(GluaBase* glua, int stack_index) -> double
{
    return glua->getDouble(stack_index);
}
auto GluaResolver<double>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isDouble(stack_index);
}
auto GluaResolver<double>::push(GluaBase* glua, double value) -> void
{
    glua->push(value);
}

auto GluaResolver<const char*>::as(GluaBase* glua, int stack_index) -> const
    char*
{
    return glua->getCharPointer(stack_index);
}
auto GluaResolver<const char*>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isCharPointer(stack_index);
}
auto GluaResolver<const char*>::push(GluaBase* glua, const char* value)
    -> void
{
    glua->push(value);
}

auto GluaResolver<std::string_view>::as(GluaBase* glua, int stack_index)
    -> std::string_view
{
    return glua->getStringView(stack_index);
}
auto GluaResolver<std::string_view>::is(GluaBase* glua, int stack_index)
    -> bool
{
    return glua->isStringView(stack_index);
}
auto GluaResolver<std::string_view>::push(GluaBase* glua,
    std::string_view value) -> void
{
    glua->push(value);
}

auto GluaResolver<std::string>::as(GluaBase* glua, int stack_index)
    -> std::string
{
    return glua->getString(stack_index);
}
auto GluaResolver<std::string>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isString(stack_index);
}
auto GluaResolver<std::string>::push(GluaBase* glua, const std::string& value)
    -> void
{
    glua->push(value);
}
} // namespace kdk::glua
