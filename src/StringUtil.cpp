#include "glua/StringUtil.h"

#include <cctype>

namespace kdk::string_util {
auto remove_all_whitespace(std::string_view input) -> std::string {
  std::string output;
  output.reserve(input.size());
  for (auto c : input) {
    if (std::isspace(c) == 0) {
      output.append({c});
    }
  }

  return output;
}

auto split(std::string_view input, std::string_view token)
    -> std::vector<std::string_view> {
  std::vector<std::string_view> output;

  auto pos = input.find(token);

  while (!input.empty() && pos != std::string_view::npos) {
    auto substr = input.substr(0, pos);
    input = input.substr(pos + 1); // skip comma

    pos = input.find(token);
    output.emplace_back(substr);
  }

  if (!input.empty()) {
    output.emplace_back(input);
  }

  return output;
}
} // namespace kdk::string_util
