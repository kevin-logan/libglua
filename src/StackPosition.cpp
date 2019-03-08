#include "glua/StackPosition.h"

#include "glua/GluaBase.h"

namespace kdk::glua
{
StackPosition::StackPosition(GluaBase* glua, int position) : m_glua(glua), m_position(position)
{
}

StackPosition::StackPosition(StackPosition&& rhs) noexcept
    : m_glua(rhs.m_glua), m_position(rhs.m_position) // trivially-copiable
{
    rhs.m_position = std::nullopt;
}

auto StackPosition::operator=(StackPosition&& rhs) noexcept -> StackPosition&
{
    m_glua     = rhs.m_glua;
    m_position = rhs.m_position; // trivially-copyable

    rhs.m_position = std::nullopt;

    return *this;
}

auto StackPosition::GetStackIndex() const -> int
{
    return m_position.value();
}

auto StackPosition::Release() -> void
{
    m_position = std::nullopt;
}

auto StackPosition::PushChild(size_t child_index) -> StackPosition
{
    return m_glua->PushChild(m_position.value(), child_index);
}

auto StackPosition::PushChild(const std::string& child_key) -> StackPosition
{
    return m_glua->PushChild(m_position.value(), child_key);
}

auto StackPosition::SafePushChild(size_t child_index) -> StackPosition
{
    return m_glua->SafePushChild(m_position.value(), child_index);
}

auto StackPosition::SafePushChild(const std::string& child_key) -> StackPosition
{
    return m_glua->SafePushChild(m_position.value(), child_key);
}

StackPosition::~StackPosition()
{
    if (m_position.has_value())
    {
        m_glua->Pop<void>();
    }
}
} // namespace kdk::glua
