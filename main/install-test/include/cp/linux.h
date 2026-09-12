#pragma once

#include <cstdint>

#if defined(__linux__)
#include <unistd.h>
#endif

namespace cp::linux {

inline constexpr bool available =
#if defined(__linux__)
    true;
#else
    false;
#endif

inline std::uint64_t process_id() noexcept {
#if defined(__linux__)
  return static_cast<std::uint64_t>(::getpid());
#else
  return 0;
#endif
}

inline long page_size() noexcept {
#if defined(__linux__)
  return ::sysconf(_SC_PAGESIZE);
#else
  return 0;
#endif
}

} // namespace cp::linux
