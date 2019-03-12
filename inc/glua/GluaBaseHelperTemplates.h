#pragma once

#include <optional>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace kdk::glua {
class GluaBase;

template <typename T> struct IsReferenceWrapper : std::false_type {};

template <typename T>
struct IsReferenceWrapper<std::reference_wrapper<T>> : std::true_type {};

template <typename T> struct IsSharedPointer : std::false_type {};

template <typename T>
struct IsSharedPointer<std::shared_ptr<T>> : std::true_type {};

template <typename T> struct GluaResolver {
  static auto as(GluaBase *glua, int stack_index) -> bool;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const T &value) -> void;
};

template <> struct GluaResolver<bool> {
  static auto as(GluaBase *glua, int stack_index) -> bool;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const T &value) -> void;
};

template <> struct GluaResolver<int8_t> {
  static auto as(GluaBase *glua, int stack_index) -> int8_t;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const T &value) -> void;
};

template <> struct GluaResolver<int16_t> {
  static auto as(GluaBase *glua, int stack_index) -> int16_t;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const T &value) -> void;
};

template <> struct GluaResolver<int32_t> {
  static auto as(GluaBase *glua, int stack_index) -> int32_t;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const T &value) -> void;
};

template <> struct GluaResolver<int64_t> {
  static auto as(GluaBase *glua, int stack_index) -> int64_t;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const T &value) -> void;
};

template <> struct GluaResolver<uint8_t> {
  static auto as(GluaBase *glua, int stack_index) -> uint8_t;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const T &value) -> void;
};

template <> struct GluaResolver<uint16_t> {
  static auto as(GluaBase *glua, int stack_index) -> uint16_t;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const T &value) -> void;
};

template <> struct GluaResolver<uint32_t> {
  static auto as(GluaBase *glua, int stack_index) -> uint32_t;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const T &value) -> void;
};

template <> struct GluaResolver<uint64_t> {
  static auto as(GluaBase *glua, int stack_index) -> uint64_t;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const T &value) -> void;
};

template <> struct GluaResolver<float> {
  static auto as(GluaBase *glua, int stack_index) -> float;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const T &value) -> void;
};

template <> struct GluaResolver<double> {
  static auto as(GluaBase *glua, int stack_index) -> double;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const T &value) -> void;
};

template <> struct GluaResolver<const char *> {
  static auto as(GluaBase *glua, int stack_index) -> const char *;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const T &value) -> void;
};

template <> struct GluaResolver<std::string_view> {
  static auto as(GluaBase *glua, int stack_index) -> std::string_view;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const T &value) -> void;
};

template <> struct GluaResolver<std::string> {
  static auto as(GluaBase *glua, int stack_index) -> std::string;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const T &value) -> void;
};

template <typename T> struct GluaResolver<std::vector<T>> {
  static auto as(GluaBase *glua, int stack_index) -> std::vector<T> {
    return glua->getArray<T>(stack_index);
  }
  static auto is(GluaBase *glua, int stack_index) -> bool {
    return glua->isArray(stack_index);
  }
  static auto push(GluaBase *glua, const std::vector<T> &value) -> void {
    return glua->push(value);
  }
};

template <typename T> struct GluaResolver<std::unordered_map<std::string, T>> {
  static auto as(GluaBase *glua, int stack_index)
      -> std::unordered_map<std::string, T> {
    return glua->getStringMap<T>(stack_index);
  }
  static auto is(GluaBase *glua, int stack_index) -> bool {
    return glua->isMap(stack_index);
  }
  static auto push(GluaBase *glua,
                   const std::unordered_map<std::string, T> &value) -> void {
    return glua->push(value);
  }
};

template <typename T> struct GluaResolver<std::optional<T>> {
  static auto as(GluaBase *glua, int stack_index) -> std::optional<T> {
    return glua->getOptional<T>(stack_index);
  }
  static auto is(GluaBase *glua, int stack_index) -> bool {
    return GluaResolver<T>::is(glua, stack_index) || glua->isNull(stack_index);
  }
  static auto push(GluaBase *glua, const T &value) -> void {
    if (value.has_value()) {
      GluaResolver<T>::push(glua, value.value());
    } else {
      glua->push(std::nullopt);
    }
  }
};

template <typename T, typename = void> struct HasCreate : std::false_type {};

template <typename T>
struct HasCreate<T, std::void_t<decltype(T::Create)>> : std::true_type {};

template <typename T> struct RegistryType { using underlying_type = T; };

template <typename T> struct RegistryType<T &> { using underlying_type = T; };

template <typename T> struct RegistryType<T *> { using underlying_type = T; };

template <typename T> struct RegistryType<std::shared_ptr<T>> {
  using underlying_type = T;
};

template <typename T> struct RegistryType<std::reference_wrapper<T>> {
  using underlying_type = T;
};
} // namespace kdk::glua
