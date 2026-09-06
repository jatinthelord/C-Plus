/*
 * C+ Networking Library
 * Copyright (c) 2026 CSP Foundation
 * Licensed under the MIT License.
 */
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace cp::networking {

struct ipv4 {
  std::array<std::uint8_t, 4> octets{};
  constexpr bool valid() const noexcept { return true; }
  std::string text() const { return std::to_string(octets[0]) + "." + std::to_string(octets[1]) + "." + std::to_string(octets[2]) + "." + std::to_string(octets[3]); }
};

struct endpoint {
  std::string host;
  std::uint16_t port = 0;
  constexpr bool valid() const noexcept { return !host.empty() && port != 0; }
};

struct packet_view {
  const std::uint8_t *data = nullptr;
  std::size_t size = 0;
  constexpr bool empty() const noexcept { return size == 0; }
  constexpr const std::uint8_t *begin() const noexcept { return data; }
  constexpr const std::uint8_t *end() const noexcept { return data + size; }
};

inline endpoint parse_endpoint(std::string_view value, std::uint16_t default_port = 0) {
  const auto separator = value.rfind(':');
  if (separator == std::string_view::npos) return {std::string(value), default_port};
  return {std::string(value.substr(0, separator)), static_cast<std::uint16_t>(std::stoi(std::string(value.substr(separator + 1))))};
}

inline std::uint16_t network_order(std::uint16_t value) noexcept { return static_cast<std::uint16_t>((value << 8) | (value >> 8)); }
inline std::uint32_t network_order(std::uint32_t value) noexcept { return ((value & 0x000000ffu) << 24) | ((value & 0x0000ff00u) << 8) | ((value & 0x00ff0000u) >> 8) | ((value & 0xff000000u) >> 24); }

} // namespace cp::networking
