/*
 * C+ Massive Library
 * Copyright (c) 2026 CSP Foundation
 *
 * MIT License. Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files, to
 * deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace cp::generated::network {

struct network_protocol_02_config {
  std::uint32_t flags = 0;
  std::size_t capacity = 0;
  std::string_view label{};
};

class network_protocol_02 {
public:
  constexpr explicit network_protocol_02(network_protocol_02_config config = {}) noexcept
      : config_(config) {}

  constexpr std::uint32_t flags() const noexcept { return config_.flags; }
  constexpr std::size_t capacity() const noexcept { return config_.capacity; }
  constexpr std::string_view label() const noexcept { return config_.label; }
  constexpr bool enabled() const noexcept { return config_.flags != 0; }
  constexpr void enable() noexcept { config_.flags |= 1u; }
  constexpr void disable() noexcept { config_.flags &= ~1u; }

  constexpr network_protocol_02_config config() const noexcept { return config_; }

private:
  network_protocol_02_config config_;
};

inline constexpr std::string_view network_protocol_02_name = "network_protocol_02";
inline constexpr std::size_t network_protocol_02_version = 2;

template <class Value> constexpr auto network_protocol_02_clamp(Value value, Value low, Value high) noexcept {
  return value < low ? low : (value > high ? high : value);
}

template <class Value> constexpr auto network_protocol_02_array(Value value) noexcept {
  std::array<Value, 4> result{{value, value, value, value}};
  return result;
}

inline constexpr bool network_protocol_02_valid(network_protocol_02_config config) noexcept {
  return config.capacity > 0 && !config.label.empty();
}

inline constexpr network_protocol_02_config network_protocol_02_defaults() noexcept {
  return {1u, 32u, network_protocol_02_name};
}

inline constexpr auto network_protocol_02_make() noexcept {
  return network_protocol_02(network_protocol_02_defaults());
}

inline constexpr std::uint32_t network_protocol_02_constant_1 = 2001u;
inline constexpr std::uint32_t network_protocol_02_constant_2 = 2002u;
inline constexpr std::uint32_t network_protocol_02_constant_3 = 2003u;
inline constexpr std::uint32_t network_protocol_02_constant_4 = 2004u;
inline constexpr std::uint32_t network_protocol_02_constant_5 = 2005u;
inline constexpr std::uint32_t network_protocol_02_constant_6 = 2006u;
inline constexpr std::uint32_t network_protocol_02_constant_7 = 2007u;
inline constexpr std::uint32_t network_protocol_02_constant_8 = 2008u;
inline constexpr std::uint32_t network_protocol_02_constant_9 = 2009u;
inline constexpr std::uint32_t network_protocol_02_constant_10 = 2010u;
inline constexpr std::uint32_t network_protocol_02_constant_11 = 2011u;
inline constexpr std::uint32_t network_protocol_02_constant_12 = 2012u;
inline constexpr std::uint32_t network_protocol_02_constant_13 = 2013u;
inline constexpr std::uint32_t network_protocol_02_constant_14 = 2014u;
inline constexpr std::uint32_t network_protocol_02_constant_15 = 2015u;
inline constexpr std::uint32_t network_protocol_02_constant_16 = 2016u;
inline constexpr std::uint32_t network_protocol_02_constant_17 = 2017u;
inline constexpr std::uint32_t network_protocol_02_constant_18 = 2018u;
inline constexpr std::uint32_t network_protocol_02_constant_19 = 2019u;
inline constexpr std::uint32_t network_protocol_02_constant_20 = 2020u;
inline constexpr std::uint32_t network_protocol_02_constant_21 = 2021u;
inline constexpr std::uint32_t network_protocol_02_constant_22 = 2022u;
inline constexpr std::uint32_t network_protocol_02_constant_23 = 2023u;
inline constexpr std::uint32_t network_protocol_02_constant_24 = 2024u;
inline constexpr std::uint32_t network_protocol_02_constant_25 = 2025u;
inline constexpr std::uint32_t network_protocol_02_constant_26 = 2026u;
inline constexpr std::uint32_t network_protocol_02_constant_27 = 2027u;
inline constexpr std::uint32_t network_protocol_02_constant_28 = 2028u;
inline constexpr std::uint32_t network_protocol_02_constant_29 = 2029u;
inline constexpr std::uint32_t network_protocol_02_constant_30 = 2030u;
inline constexpr std::uint32_t network_protocol_02_constant_31 = 2031u;
inline constexpr std::uint32_t network_protocol_02_constant_32 = 2032u;
inline constexpr std::uint32_t network_protocol_02_constant_33 = 2033u;
inline constexpr std::uint32_t network_protocol_02_constant_34 = 2034u;
inline constexpr std::uint32_t network_protocol_02_constant_35 = 2035u;
inline constexpr std::uint32_t network_protocol_02_constant_36 = 2036u;
inline constexpr std::uint32_t network_protocol_02_constant_37 = 2037u;
inline constexpr std::uint32_t network_protocol_02_constant_38 = 2038u;
inline constexpr std::uint32_t network_protocol_02_constant_39 = 2039u;
inline constexpr std::uint32_t network_protocol_02_constant_40 = 2040u;
inline constexpr std::uint32_t network_protocol_02_constant_41 = 2041u;
inline constexpr std::uint32_t network_protocol_02_constant_42 = 2042u;
inline constexpr std::uint32_t network_protocol_02_constant_43 = 2043u;
inline constexpr std::uint32_t network_protocol_02_constant_44 = 2044u;
inline constexpr std::uint32_t network_protocol_02_constant_45 = 2045u;
inline constexpr std::uint32_t network_protocol_02_constant_46 = 2046u;
inline constexpr std::uint32_t network_protocol_02_constant_47 = 2047u;
inline constexpr std::uint32_t network_protocol_02_constant_48 = 2048u;
inline constexpr std::uint32_t network_protocol_02_constant_49 = 2049u;
inline constexpr std::uint32_t network_protocol_02_constant_50 = 2050u;
inline constexpr std::uint32_t network_protocol_02_constant_51 = 2051u;
inline constexpr std::uint32_t network_protocol_02_constant_52 = 2052u;
inline constexpr std::uint32_t network_protocol_02_constant_53 = 2053u;
inline constexpr std::uint32_t network_protocol_02_constant_54 = 2054u;
inline constexpr std::uint32_t network_protocol_02_constant_55 = 2055u;
inline constexpr std::uint32_t network_protocol_02_constant_56 = 2056u;
inline constexpr std::uint32_t network_protocol_02_constant_57 = 2057u;
inline constexpr std::uint32_t network_protocol_02_constant_58 = 2058u;
inline constexpr std::uint32_t network_protocol_02_constant_59 = 2059u;
inline constexpr std::uint32_t network_protocol_02_constant_60 = 2060u;
inline constexpr std::uint32_t network_protocol_02_constant_61 = 2061u;
inline constexpr std::uint32_t network_protocol_02_constant_62 = 2062u;
inline constexpr std::uint32_t network_protocol_02_constant_63 = 2063u;
inline constexpr std::uint32_t network_protocol_02_constant_64 = 2064u;
inline constexpr std::uint32_t network_protocol_02_constant_65 = 2065u;
inline constexpr std::uint32_t network_protocol_02_constant_66 = 2066u;
inline constexpr std::uint32_t network_protocol_02_constant_67 = 2067u;
inline constexpr std::uint32_t network_protocol_02_constant_68 = 2068u;
inline constexpr std::uint32_t network_protocol_02_constant_69 = 2069u;
inline constexpr std::uint32_t network_protocol_02_constant_70 = 2070u;
inline constexpr std::uint32_t network_protocol_02_constant_71 = 2071u;
inline constexpr std::uint32_t network_protocol_02_constant_72 = 2072u;
inline constexpr std::uint32_t network_protocol_02_constant_73 = 2073u;
inline constexpr std::uint32_t network_protocol_02_constant_74 = 2074u;
inline constexpr std::uint32_t network_protocol_02_constant_75 = 2075u;
inline constexpr std::uint32_t network_protocol_02_constant_76 = 2076u;
inline constexpr std::uint32_t network_protocol_02_constant_77 = 2077u;
inline constexpr std::uint32_t network_protocol_02_constant_78 = 2078u;
inline constexpr std::uint32_t network_protocol_02_constant_79 = 2079u;
inline constexpr std::uint32_t network_protocol_02_constant_80 = 2080u;
inline constexpr std::uint32_t network_protocol_02_constant_81 = 2081u;
inline constexpr std::uint32_t network_protocol_02_constant_82 = 2082u;
inline constexpr std::uint32_t network_protocol_02_constant_83 = 2083u;
inline constexpr std::uint32_t network_protocol_02_constant_84 = 2084u;
inline constexpr std::uint32_t network_protocol_02_constant_85 = 2085u;
inline constexpr std::uint32_t network_protocol_02_constant_86 = 2086u;
inline constexpr std::uint32_t network_protocol_02_constant_87 = 2087u;
inline constexpr std::uint32_t network_protocol_02_constant_88 = 2088u;
inline constexpr std::uint32_t network_protocol_02_constant_89 = 2089u;
inline constexpr std::uint32_t network_protocol_02_constant_90 = 2090u;
inline constexpr std::uint32_t network_protocol_02_constant_91 = 2091u;
inline constexpr std::uint32_t network_protocol_02_constant_92 = 2092u;
inline constexpr std::uint32_t network_protocol_02_constant_93 = 2093u;
inline constexpr std::uint32_t network_protocol_02_constant_94 = 2094u;
inline constexpr std::uint32_t network_protocol_02_constant_95 = 2095u;
inline constexpr std::uint32_t network_protocol_02_constant_96 = 2096u;
inline constexpr std::uint32_t network_protocol_02_constant_97 = 2097u;
inline constexpr std::uint32_t network_protocol_02_constant_98 = 2098u;
inline constexpr std::uint32_t network_protocol_02_constant_99 = 2099u;
inline constexpr std::uint32_t network_protocol_02_constant_100 = 2100u;

} // namespace cp::generated::network
