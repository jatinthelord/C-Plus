#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

namespace cp::kernel {

enum class status : std::uint8_t { ok, invalid_argument, unavailable };

struct launch_shape {
  std::size_t grid = 1;
  std::size_t block = 1;
  std::size_t shared_bytes = 0;
};

constexpr std::size_t linear_index(launch_shape shape, std::size_t block_id,
                                   std::size_t thread_id) noexcept {
  return block_id * shape.block + thread_id;
}

template <class Function>
constexpr void for_each_thread(launch_shape shape, Function function) {
  for (std::size_t block = 0; block < shape.grid; ++block)
    for (std::size_t thread = 0; thread < shape.block; ++thread)
      function(linear_index(shape, block, thread));
}

template <class T>
constexpr T grid_stride_index(T index, T stride, T size) noexcept {
  return index < size ? index : size;
}

inline constexpr bool available = false;

} // namespace cp::kernel
