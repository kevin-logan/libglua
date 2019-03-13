#pragma once

// includes this for IDE completion, but this file must actually be included
// FROM GluaBase.tcc, so at this point GluaBase.h must always have been included
// anyway
#include "glua/GluaBase.h"

#include <optional>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace kdk::glua {
template <typename T> struct GluaResolver {
  static auto as(GluaBase *glua, int stack_index) -> T;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, T value)
      -> void; // base resolver takes by value, specializations may take const
               // references if preferred
};

template <> struct GluaResolver<bool> {
  static auto as(GluaBase *glua, int stack_index) -> bool;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, bool value) -> void;
};

template <> struct GluaResolver<int8_t> {
  static auto as(GluaBase *glua, int stack_index) -> int8_t;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, int8_t value) -> void;
};

template <> struct GluaResolver<int16_t> {
  static auto as(GluaBase *glua, int stack_index) -> int16_t;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, int16_t value) -> void;
};

template <> struct GluaResolver<int32_t> {
  static auto as(GluaBase *glua, int stack_index) -> int32_t;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, int32_t value) -> void;
};

template <> struct GluaResolver<int64_t> {
  static auto as(GluaBase *glua, int stack_index) -> int64_t;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, int64_t value) -> void;
};

template <> struct GluaResolver<uint8_t> {
  static auto as(GluaBase *glua, int stack_index) -> uint8_t;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, uint8_t value) -> void;
};

template <> struct GluaResolver<uint16_t> {
  static auto as(GluaBase *glua, int stack_index) -> uint16_t;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, uint16_t value) -> void;
};

template <> struct GluaResolver<uint32_t> {
  static auto as(GluaBase *glua, int stack_index) -> uint32_t;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, uint32_t value) -> void;
};

template <> struct GluaResolver<uint64_t> {
  static auto as(GluaBase *glua, int stack_index) -> uint64_t;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, uint64_t value) -> void;
};

template <> struct GluaResolver<float> {
  static auto as(GluaBase *glua, int stack_index) -> float;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, float value) -> void;
};

template <> struct GluaResolver<double> {
  static auto as(GluaBase *glua, int stack_index) -> double;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, double value) -> void;
};

template <> struct GluaResolver<const char *> {
  static auto as(GluaBase *glua, int stack_index) -> const char *;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const char *value) -> void;
};

template <> struct GluaResolver<std::string_view> {
  static auto as(GluaBase *glua, int stack_index) -> std::string_view;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, std::string_view value) -> void;
};

template <> struct GluaResolver<std::string> {
  static auto as(GluaBase *glua, int stack_index) -> std::string;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const std::string &value) -> void;
};

template <typename T> struct GluaResolver<T *> {
  static auto as(GluaBase *glua, int stack_index) -> T *;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, T *value) -> void;
};

template <typename T> struct GluaResolver<std::reference_wrapper<T>> {
  static auto as(GluaBase *glua, int stack_index) -> std::reference_wrapper<T>;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, std::reference_wrapper<T> value) -> void;
};

template <typename T> struct GluaResolver<std::shared_ptr<T>> {
  static auto as(GluaBase *glua, int stack_index) -> std::shared_ptr<T>;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, std::shared_ptr<T> value) -> void;
};

template <typename T> struct GluaResolver<std::vector<T>> {
  static auto as(GluaBase *glua, int stack_index) -> std::vector<T>;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const std::vector<T> &value) -> void;
};

template <typename T> struct GluaResolver<std::unordered_map<std::string, T>> {
  static auto as(GluaBase *glua, int stack_index)
      -> std::unordered_map<std::string, T>;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua,
                   const std::unordered_map<std::string, T> &value) -> void;
};

template <typename T> struct GluaResolver<std::optional<T>> {
  static auto as(GluaBase *glua, int stack_index) -> std::optional<T>;
  static auto is(GluaBase *glua, int stack_index) -> bool;
  static auto push(GluaBase *glua, const std::optional<T> &value) -> void;
};

template <typename T, typename = void> struct HasCreate : std::false_type {};

template <typename T>
struct HasCreate<T, std::void_t<decltype(T::Create)>> : std::true_type {};
} // namespace kdk::glua

#include "glua/GluaBaseHelperTemplates.tcc"
