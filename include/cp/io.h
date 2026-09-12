#pragma once
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace cp {
inline std::optional<std::string> read_all(std::string_view path) {
  std::ifstream input(std::string(path), std::ios::binary);
  if (!input)
    return std::nullopt;
  std::ostringstream out;
  out << input.rdbuf();
  return out.str();
}

inline bool write_all(std::string_view path, std::string_view data) {
  std::ofstream output(std::string(path), std::ios::binary);
  if (!output)
    return false;
  output.write(data.data(), static_cast<std::streamsize>(data.size()));
  return static_cast<bool>(output);
}
} // namespace cp
