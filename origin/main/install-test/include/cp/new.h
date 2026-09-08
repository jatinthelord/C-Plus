#pragma once

#include <cstddef>
#include <memory>
#include <memory_resource>
#include <new>
#include <utility>

namespace cp {

template <class T, class... Arguments>
constexpr T *construct(void *storage, Arguments &&...arguments) {
  return std::construct_at(static_cast<T *>(storage),
                           std::forward<Arguments>(arguments)...);
}

template <class T>
constexpr void destroy(T *object) noexcept {
  std::destroy_at(object);
}

class arena {
public:
  explicit arena(std::size_t initial_bytes = 4096)
      : initial_(std::make_unique<std::byte[]>(initial_bytes)),
        resource_(initial_.get(), initial_bytes) {}

  arena(const arena &) = delete;
  arena &operator=(const arena &) = delete;

  template <class T, class... Arguments>
  T *create(Arguments &&...arguments) {
    std::pmr::polymorphic_allocator<T> allocator(&resource_);
    T *memory = allocator.allocate(1);
    try {
      return std::construct_at(memory, std::forward<Arguments>(arguments)...);
    } catch (...) {
      allocator.deallocate(memory, 1);
      throw;
    }
  }

  void release() noexcept { resource_.release(); }
  std::pmr::memory_resource *resource() noexcept { return &resource_; }

private:
  std::unique_ptr<std::byte[]> initial_;
  std::pmr::monotonic_buffer_resource resource_;
};

} // namespace cp
