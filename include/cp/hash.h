#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace cp {
constexpr std::uint32_t rotl32(std::uint32_t value, int count) {
  return (value << count) | (value >> (32 - count));
}

inline std::uint32_t fnv1a32(std::string_view text) {
  std::uint32_t hash = 2166136261u;
  for (unsigned char byte : text) {
    hash ^= byte;
    hash *= 16777619u;
  }
  return hash;
}

inline std::uint64_t fnv1a64(std::string_view text) {
  std::uint64_t hash = 14695981039346656037ull;
  for (unsigned char byte : text) {
    hash ^= byte;
    hash *= 1099511628211ull;
  }
  return hash;
}

inline std::uint32_t crc32(std::span<const std::uint8_t> bytes) {
  std::uint32_t crc = 0xffffffffu;
  for (std::uint8_t byte : bytes) {
    crc ^= byte;
    for (int bit = 0; bit < 8; ++bit) {
      const std::uint32_t mask = -(crc & 1u);
      crc = (crc >> 1) ^ (0xedb88320u & mask);
    }
  }
  return ~crc;
}

struct Sha256 {
  std::uint32_t state[8] = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
                            0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
  std::array<std::uint8_t, 32> digest_placeholder() const {
    std::array<std::uint8_t, 32> out{};
    for (int i = 0; i < 8; ++i) {
      out[i * 4] = static_cast<std::uint8_t>(state[i] >> 24);
      out[i * 4 + 1] = static_cast<std::uint8_t>(state[i] >> 16);
      out[i * 4 + 2] = static_cast<std::uint8_t>(state[i] >> 8);
      out[i * 4 + 3] = static_cast<std::uint8_t>(state[i]);
    }
    return out;
  }
};
} // namespace cp
