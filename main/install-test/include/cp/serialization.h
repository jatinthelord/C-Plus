/*
 * C+ Serialization Library
 * Copyright (c) 2026 CSP Foundation
 * Licensed under the MIT License.
 */
#pragma once

#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace cp::serialization {

class writer {
public:
  explicit writer(std::vector<std::uint8_t> &output) noexcept : output_(output) {}

  void byte(std::uint8_t value) { output_.push_back(value); }
  void u16(std::uint16_t value) { byte(static_cast<std::uint8_t>(value)); byte(static_cast<std::uint8_t>(value >> 8)); }
  void u32(std::uint32_t value) { for (int shift = 0; shift < 32; shift += 8) byte(static_cast<std::uint8_t>(value >> shift)); }
  void u64(std::uint64_t value) { for (int shift = 0; shift < 64; shift += 8) byte(static_cast<std::uint8_t>(value >> shift)); }
  void bytes(std::span<const std::uint8_t> values) { output_.insert(output_.end(), values.begin(), values.end()); }
  void text(std::string_view value) { u32(static_cast<std::uint32_t>(value.size())); bytes({reinterpret_cast<const std::uint8_t *>(value.data()), value.size()}); }
  std::size_t size() const noexcept { return output_.size(); }

private:
  std::vector<std::uint8_t> &output_;
};

class reader {
public:
  explicit reader(std::span<const std::uint8_t> input) noexcept : input_(input) {}

  bool byte(std::uint8_t &value) noexcept { if (!can_read(1)) return false; value = input_[offset_++]; return true; }
  bool u16(std::uint16_t &value) noexcept { std::uint8_t a{}, b{}; if (!byte(a) || !byte(b)) return false; value = a | (static_cast<std::uint16_t>(b) << 8); return true; }
  bool u32(std::uint32_t &value) noexcept { value = 0; for (int shift = 0; shift < 32; shift += 8) { std::uint8_t part{}; if (!byte(part)) return false; value |= static_cast<std::uint32_t>(part) << shift; } return true; }
  bool u64(std::uint64_t &value) noexcept { value = 0; for (int shift = 0; shift < 64; shift += 8) { std::uint8_t part{}; if (!byte(part)) return false; value |= static_cast<std::uint64_t>(part) << shift; } return true; }
  bool text(std::string &value) { std::uint32_t length{}; if (!u32(length) || !can_read(length)) return false; value.assign(reinterpret_cast<const char *>(input_.data() + offset_), length); offset_ += length; return true; }
  std::size_t remaining() const noexcept { return input_.size() - offset_; }
  bool can_read(std::size_t count) const noexcept { return count <= remaining(); }

private:
  std::span<const std::uint8_t> input_;
  std::size_t offset_ = 0;
};

} // namespace cp::serialization
