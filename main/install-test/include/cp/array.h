#pragma once

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace cp {

template <class T, std::size_t Size>
using array = std::array<T, Size>;

template <std::size_t Size>
using byte_array = std::array<std::byte, Size>;

template <class T, std::size_t Size>
constexpr array<T, Size> filled_array(const T &value) {
  array<T, Size> result{};
  result.fill(value);
  return result;
}

template <class T, class... Values>
constexpr auto make_array(T first, Values... values)
    -> array<std::common_type_t<T, Values...>, 1 + sizeof...(Values)> {
  using value_type = std::common_type_t<T, Values...>;
  return {static_cast<value_type>(first), static_cast<value_type>(values)...};
}

template <class T, std::size_t Size>
constexpr bool in_bounds(const array<T, Size> &, std::size_t index) noexcept {
  return index < Size;
}

} // namespace cp
