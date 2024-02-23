#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <variant>

namespace glua {
using basic_type_variant = std::variant<
    int8_t, int16_t, int32_t, int64_t,
    uint8_t, uint16_t, uint32_t, uint64_t,
    float, double,
    bool,
    std::string>;

template <typename T>
concept basic_type = requires(T, basic_type_variant v) {
    {
        std::get<T>(v)
    };
};

template <typename T>
concept decays_to_basic_type = basic_type<std::decay_t<T>>;

struct any_impl {
    virtual ~any_impl() = default;
};

struct any {
    std::unique_ptr<any_impl> impl_;
};
}