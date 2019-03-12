#pragma once

#include "glua/GluaBase.h"

namespace kdk::glua {

template <typename T>
auto GluaResolver<T>::as(GluaBase *glua, int stack_index) -> bool {}

template <typename T>
auto GluaResolver<T>::is(GluaBase *glua, int stack_index) -> bool {}

template <typename T> static auto push(GluaBase *glua, const T &value) -> void {
  using RawT = std::decay_t<T>;
  if constexpr (std::is_enum<RawT>::value) {
    GluaResolver<uint64_t>::push(glua, static_cast<uint64_t>(value));
  } else {
    using UnderlyingType = typename RegistryType<RawT>::underlying_type;

    auto unique_name_opt = glua->getUniqueClassName<RawT>();

    if (unique_name_opt.has_value()) {
      // create storage, hand off to implementation
      if constexpr (std::is_pointer<RawT>::value) {
        glua->pushUserType(
            unique_name_opt.value(),
            std::make_unique<ManagedTypeRawPtr<UnderlyingType>>(value));
      } else if constexpr (IsReferenceWrapper<RawT>::value) {
        glua->pushUserType(unique_name_opt.value(),
                           std::make_unique<ManagedTypeRawPtr<UnderlyingType>>(
                               static_cast<UnderlyingType *>(&value.get())));
      } else if constexpr (IsSharedPointer<RawT>::value) {
        glua->pushUserType(
            unique_name_opt.value(),
            std::make_unique<ManagedTypeSharedPtr<UnderlyingType>>(value));
      } else {
        // by value
        pushUserType(
            unique_name_opt.value(),
            std::make_unique<
                ManagedTypeStackAllocated<std::remove_reference_t<Type>>>(
                std::forward<Type>(custom_type)));
      }
    } else {
      throw exceptions::GluaBaseException(
          "Attempted to push unregistered type");
    }
  }
}

} // namespace kdk::glua
