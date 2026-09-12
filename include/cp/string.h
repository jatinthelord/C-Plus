#pragma once
#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace cp {
inline std::string trim(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() &&
         std::isspace(static_cast<unsigned char>(text[begin])))
    ++begin;
  std::size_t end = text.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])))
    --end;
  return std::string(text.substr(begin, end - begin));
}

inline std::string to_lower(std::string_view text) {
  std::string out(text);
  for (char &ch : out)
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  return out;
}

inline std::vector<std::string> split(std::string_view text, char sep) {
  std::vector<std::string> out;
  std::string current;
  for (char ch : text) {
    if (ch == sep) {
      out.push_back(current);
      current.clear();
    } else
      current.push_back(ch);
  }
  out.push_back(current);
  return out;
}

inline bool starts_with(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}
} // namespace cp
