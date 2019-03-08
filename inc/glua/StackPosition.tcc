#include "glua/StackPosition.h"

namespace kdk::glua
{
template<typename Type>
auto StackPosition::As() -> Type
{
    return m_glua->As<Type>(m_position.value());
}

template<typename Type>
auto StackPosition::Is() -> bool
{
    return m_glua->Is<Type>(m_position.value());
}

template<typename Type>
auto StackPosition::Get() -> Type
{
    return m_glua->Get<Type>(m_position.value());
}
} // namespace kdk::glua
