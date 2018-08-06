#include "glua/GluaCallable.h"

namespace kdk::glua
{
template<typename Functor, typename... Params>
GluaCallable<Functor, Params...>::GluaCallable(std::shared_ptr<Glua> glua, Functor functor)
    : DeferredArgumentCallable<Glua, Functor, Params...>(std::move(functor)), m_glua(glua)
{
}

template<typename Functor, typename... Params>
auto GluaCallable<Functor, Params...>::GetGlua() const -> std::shared_ptr<Glua>
{
    return m_glua;
}

template<typename Functor, typename... Params>
auto GluaCallable<Functor, Params...>::GetImplementationData() const -> void*
{
    return m_glua.get();
}

} // namespace kdk::glua
