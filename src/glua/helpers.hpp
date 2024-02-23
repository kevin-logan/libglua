#pragma once

#include <type_traits>
#include <unordered_map>
#include <vector>

namespace glua {
template <typename T, typename U>
concept decays_to = std::same_as<std::decay_t<T>, U>;

template <typename T>
concept decays_to_integral = std::integral<std::decay_t<T>>;

template <typename T>
struct is_vector : std::false_type { };

template <typename T>
struct is_vector<std::vector<T>> : std::true_type { };

template <typename T>
concept decays_to_vector = !is_vector<T>::value && is_vector<std::decay_t<T>>::value;

template <typename T>
struct is_unordered_map : std::false_type { };

template <typename T, typename U>
struct is_unordered_map<std::unordered_map<T, U>> : std::true_type { };

template <typename T>
concept decays_to_unordered_map = !is_unordered_map<T>::value && is_unordered_map<std::decay_t<T>>::value;
}