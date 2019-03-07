#pragma once

#include <stdexcept>
#include <string>

namespace kdk::exceptions
{
#define KDK_DERIVED_EXCEPTION(name, base)                                 \
    class name : public base                                              \
    {                                                                     \
    public:                                                               \
        template<typename... Params>                                      \
        explicit name(Params&&... ps) : base(std::forward<Params>(ps)...) \
        {                                                                 \
        }                                                                 \
    }

#define KDK_EXCEPTION(name) KDK_DERIVED_EXCEPTION(name, std::runtime_error)

KDK_EXCEPTION(GluaBaseException);
KDK_EXCEPTION(GluaTypeException);
KDK_EXCEPTION(LuaException);

} // namespace kdk::exceptions
