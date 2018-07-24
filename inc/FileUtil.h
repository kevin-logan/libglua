#pragma once

#include <string>
#include <string_view>

namespace kdk::file_util
{
auto read_all(std::string_view filename) -> std::string;

auto write_all(std::string_view filename, std::string_view data) -> void;

auto append_all(std::string_view filename, std::string_view data) -> void;

} // namespace kdk::file_util
