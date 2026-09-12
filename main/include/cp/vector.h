#pragma once
#include <memory_resource>
#include <span>
#include <vector>

namespace cp {
template <class T, class Allocator = std::allocator<T>>
using dynamic_vector = std::vector<T, Allocator>;

template <class T> using vector_view = std::span<T>;
template <class T> using const_vector_view = std::span<const T>;

template <class T> using pmr_vector = std::pmr::vector<T>;

template <class T, class Allocator>
vector_view<T> view(std::vector<T, Allocator> &values) {
  return {values.data(), values.size()};
}

template <class T, class Allocator>
const_vector_view<T> view(const std::vector<T, Allocator> &values) {
  return {values.data(), values.size()};
}
} // namespace cp
