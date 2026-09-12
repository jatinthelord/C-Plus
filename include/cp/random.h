#pragma once
#include <cstdint>

namespace cp {
struct XorShift64 {
  std::uint64_t state = 0x9e3779b97f4a7c15ull;
  std::uint64_t next() {
    std::uint64_t x = state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state = x;
    return x;
  }
  int next_int(int low, int high) {
    if (high <= low)
      return low;
    return low + static_cast<int>(next() % static_cast<std::uint64_t>(high - low));
  }
  double next_unit() {
    return (next() >> 11) * (1.0 / 9007199254740992.0);
  }
};
} // namespace cp
