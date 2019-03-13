#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace kdk::string_util {
auto remove_all_whitespace(std::string_view input) -> std::string;
auto split(std::string_view input, std::string_view token) -> std::vector<std::string_view>;
} // namespace kdk::string_util
