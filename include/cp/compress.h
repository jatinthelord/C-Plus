#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace cp {
inline std::vector<std::uint8_t> rle_encode(std::string_view data) {
  std::vector<std::uint8_t> out;
  std::size_t i = 0;
  while (i < data.size()) {
    std::size_t run = 1;
    while (i + run < data.size() && data[i + run] == data[i] && run < 255)
      ++run;
    out.push_back(static_cast<std::uint8_t>(run));
    out.push_back(static_cast<std::uint8_t>(data[i]));
    i += run;
  }
  return out;
}

inline std::string rle_decode(const std::vector<std::uint8_t> &data) {
  std::string out;
  for (std::size_t i = 0; i + 1 < data.size(); i += 2)
    out.append(data[i], static_cast<char>(data[i + 1]));
  return out;
}
} // namespace cp
