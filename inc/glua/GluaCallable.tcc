#pragma once

#include "glua/GluaCallable.h"

namespace kdk::glua {
template <typename Functor, typename... Params>
GluaCallable<Functor, Params...>::GluaCallable(GluaBase* glua, Functor functor)
    : DeferredArgumentCallable<GluaBase, Functor, Params...>(std::move(functor))
    , m_glua(glua)
{
}

template <typename Functor, typename... Params>
auto GluaCallable<Functor, Params...>::GetGlua() const -> GluaBase*
{
    return m_glua;
}

template <typename Functor, typename... Params>
auto GluaCallable<Functor, Params...>::GetImplementationData() const -> void*
{
    return m_glua;
}

} // namespace kdk::glua
