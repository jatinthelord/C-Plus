#pragma once
#include <regex>
#include <string>
#include <string_view>
#include <vector>

namespace cp {
inline bool matches(std::string_view text, std::string_view pattern) {
  return std::regex_search(std::string(text),
                           std::regex(std::string(pattern)));
}

inline std::vector<std::string> find_all(std::string_view text,
                                         std::string_view pattern) {
  std::vector<std::string> out;
  const std::string haystack(text);
  const std::regex compiled(std::string(pattern));
  auto begin = std::sregex_iterator(haystack.begin(), haystack.end(), compiled);
  const auto end = std::sregex_iterator();
  for (auto it = begin; it != end; ++it)
    out.push_back(it->str());
  return out;
}

inline std::string replace_all(std::string_view text, std::string_view pattern,
                               std::string_view with) {
  return std::regex_replace(std::string(text), std::regex(std::string(pattern)),
                            std::string(with));
}
} // namespace cp
