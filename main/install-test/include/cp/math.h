#pragma once
#include <bit>
#include <cmath>
#include <concepts>
#include <limits>
#include <numbers>

namespace cp {
template <class T> constexpr T min(T left, T right) {
  return right < left ? right : left;
}
template <class T> constexpr T max(T left, T right) {
  return left < right ? right : left;
}
template <class T> constexpr T clamp(T value, T low, T high) {
  return value < low ? low : (high < value ? high : value);
}
template <class T, class U> constexpr auto lerp(T from, T to, U amount) {
  return from + (to - from) * amount;
}
template <std::integral T> constexpr bool is_power_of_two(T value) {
  return value > 0 && (value & (value - 1)) == 0;
}
template <std::unsigned_integral T>
constexpr T rotate_left(T value, int count) {
  return std::rotl(value, count);
}
template <std::unsigned_integral T>
constexpr T rotate_right(T value, int count) {
  return std::rotr(value, count);
}
template <std::unsigned_integral T> constexpr int population_count(T value) {
  return std::popcount(value);
}
template <std::floating_point T> constexpr T pi = std::numbers::pi_v<T>;
template <std::floating_point T> constexpr T tau = T(2) * pi<T>;
} // namespace cp
