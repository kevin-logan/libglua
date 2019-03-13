#pragma once

#include "glua/ICallable.h"

#include <memory>

namespace kdk::glua {
class GluaBase;

template <typename Functor, typename... Params>
class GluaCallable : public DeferredArgumentCallable<GluaBase, Functor, Params...> {
public:
    GluaCallable(GluaBase* glua, Functor functor);
    GluaCallable(const GluaCallable&) = default;
    GluaCallable(GluaCallable&&) noexcept = default;

    auto operator=(const GluaCallable&) -> GluaCallable& = default;
    auto operator=(GluaCallable&&) noexcept -> GluaCallable& = default;

    auto GetGlua() const -> GluaBase*;
    auto GetImplementationData() const -> void* override;

    ~GluaCallable() override = default;

private:
    GluaBase* m_glua;
};

} // namespace kdk::glua

#include "glua/GluaCallable.tcc"
