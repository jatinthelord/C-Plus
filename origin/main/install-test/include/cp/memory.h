#pragma once
#include <cstdlib>
#include <cstring>
#include <memory>
#include <span>
#include <utility>

namespace cp {
template <class T> using owner = std::unique_ptr<T>;
template <class T> using array_owner = std::unique_ptr<T[]>;
template <class T> using shared_owner = std::shared_ptr<T>;
template <class T> using weak_owner = std::weak_ptr<T>;
template <class T, class... Arguments>
owner<T> make_owner(Arguments &&...arguments) {
  return std::make_unique<T>(std::forward<Arguments>(arguments)...);
}
template <class T, class... Arguments>
shared_owner<T> make_shared_owner(Arguments &&...arguments) {
  return std::make_shared<T>(std::forward<Arguments>(arguments)...);
}
template <class T> array_owner<T> make_array(std::size_t count) {
  return std::make_unique<T[]>(count);
}
template <class T> std::span<T> view(T *data, std::size_t count) {
  return {data, count};
}
inline void copy_bytes(void *destination, const void *source,
                       std::size_t bytes) {
  std::memcpy(destination, source, bytes);
}
inline void zero_bytes(void *destination, std::size_t bytes) {
  std::memset(destination, 0, bytes);
}
inline bool equal_bytes(const void *left, const void *right,
                        std::size_t bytes) noexcept {
  return std::memcmp(left, right, bytes) == 0;
}
constexpr std::size_t align_up(std::size_t value,
                               std::size_t alignment) noexcept {
  return alignment == 0 ? value : (value + alignment - 1) / alignment * alignment;
}
} // namespace cp
