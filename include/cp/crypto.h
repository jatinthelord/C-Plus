#pragma once
#include <cp/hash.h>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace cp {
inline std::uint64_t splitmix64(std::uint64_t value) {
  value += 0x9e3779b97f4a7c15ull;
  value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ull;
  value = (value ^ (value >> 27)) * 0x94d049bb133111ebull;
  return value ^ (value >> 31);
}

inline std::vector<std::uint8_t> xor_bytes(std::string_view data,
                                           std::string_view key) {
  std::vector<std::uint8_t> out(data.size());
  if (key.empty())
    return out;
  for (std::size_t i = 0; i < data.size(); ++i)
    out[i] = static_cast<std::uint8_t>(data[i]) ^
             static_cast<std::uint8_t>(key[i % key.size()]);
  return out;
}

inline std::uint64_t keyed_hash(std::string_view data, std::string_view key) {
  return fnv1a64(std::string(key) + std::string(data));
}
} // namespace cp
