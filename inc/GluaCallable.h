#pragma once

#include "ICallable.h"

#include <memory>

namespace kdk::glua
{
class Glua;

template<typename Functor, typename... Params>
class GluaCallable : public DeferredArgumentCallable<Glua, Functor, Params...>
{
public:
    GluaCallable(std::shared_ptr<Glua> glua, Functor functor);
    GluaCallable(GluaCallable&&) = default;

    auto GetGlua() const -> std::shared_ptr<Glua>;
    auto GetImplementationData() const -> void* override;

    ~GluaCallable() override = default;

private:
    std::shared_ptr<Glua> m_glua;
};

} // namespace kdk::glua

#include "GluaCallable.tcc"
