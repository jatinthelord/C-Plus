#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <initializer_list>

namespace cp {
template <class T, std::size_t N> struct vector {
  std::array<T, N> lanes{};
  constexpr vector() = default;
  constexpr vector(std::initializer_list<T> values) {
    std::size_t index = 0;
    for (T value : values)
      if (index < N)
        lanes[index++] = value;
  }
  constexpr T &operator[](std::size_t index) { return lanes[index]; }
  constexpr const T &operator[](std::size_t index) const {
    return lanes[index];
  }
  constexpr vector operator+(const vector &other) const {
    vector result;
    for (std::size_t i = 0; i < N; ++i)
      result[i] = lanes[i] + other[i];
    return result;
  }
  constexpr vector operator-(const vector &other) const {
    vector result;
    for (std::size_t i = 0; i < N; ++i)
      result[i] = lanes[i] - other[i];
    return result;
  }
  constexpr T dot(const vector &other) const {
    T result{};
    for (std::size_t i = 0; i < N; ++i)
      result += lanes[i] * other[i];
    return result;
  }
};
using float2 = vector<float, 2>;
using float3 = vector<float, 3>;
using float4 = vector<float, 4>;
using int2 = vector<int, 2>;
using int3 = vector<int, 3>;
using int4 = vector<int, 4>;
} // namespace cp
