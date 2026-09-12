#pragma once
#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace cp {
inline void check(bool condition, std::string_view message) {
  if (condition)
    return;
  std::fprintf(stderr, "check failed: %.*s\n",
               static_cast<int>(message.size()), message.data());
  std::abort();
}

inline void expect_eq(long long left, long long right, std::string_view name) {
  if (left == right)
    return;
  std::fprintf(stderr, "%.*s: expected %lld == %lld\n",
               static_cast<int>(name.size()), name.data(), left, right);
  std::abort();
}
} // namespace cp
