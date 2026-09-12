/*
 * C+ Filesystem Library
 * Copyright (c) 2026 CSP Foundation
 * Licensed under the MIT License.
 */
#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

namespace cp::fs {

namespace native = std::filesystem;
using path = native::path;

inline bool exists(const path &value) noexcept { return native::exists(value); }
inline bool regular_file(const path &value) noexcept { return native::is_regular_file(value); }
inline bool directory(const path &value) noexcept { return native::is_directory(value); }
inline bool create_directories(const path &value) { return native::create_directories(value); }
inline bool remove(const path &value) { return native::remove(value); }
inline std::uintmax_t size(const path &value) { return native::file_size(value); }
inline path absolute(const path &value) { return native::absolute(value); }
inline path join(const path &left, const path &right) { return left / right; }

inline std::optional<std::string> read_text(const path &value) {
  std::ifstream input(value, std::ios::binary);
  if (!input) return std::nullopt;
  return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

inline bool write_text(const path &value, std::string_view text) {
  std::ofstream output(value, std::ios::binary);
  if (!output) return false;
  output.write(text.data(), static_cast<std::streamsize>(text.size()));
  return static_cast<bool>(output);
}

struct file_info {
  path name;
  bool is_directory = false;
  std::uintmax_t bytes = 0;
};

inline file_info inspect(const path &value) {
  const bool is_directory = directory(value);
  return {value, is_directory, is_directory ? 0 : size(value)};
}

template <class Function> void for_each_entry(const path &root, Function function) {
  if (!directory(root)) return;
  for (const auto &entry : native::directory_iterator(root)) function(inspect(entry.path()));
}

} // namespace cp::fs
