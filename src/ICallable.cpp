#include "glua/ICallable.h"

namespace kdk
{
Callable::Callable(std::unique_ptr<ICallable> callable) : m_callable(std::move(callable))
{
}

auto Callable::Call() const -> void
{
    m_callable->Call();
}

auto Callable::HasReturn() const -> bool
{
    return m_callable->HasReturn();
}

auto Callable::GetImplementationData() const -> void*
{
    return m_callable->GetImplementationData();
}

auto Callable::AcquireCallable() && -> std::unique_ptr<ICallable>
{
    return std::move(m_callable);
}

} // namespace kdk
