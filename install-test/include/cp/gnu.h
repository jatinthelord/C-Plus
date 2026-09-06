#pragma once

#if defined(__GNUC__) || defined(__clang__)
#define CP_GNU_LIKELY(value) __builtin_expect(!!(value), 1)
#define CP_GNU_UNLIKELY(value) __builtin_expect(!!(value), 0)
#define CP_GNU_ALWAYS_INLINE inline __attribute__((always_inline))
#define CP_GNU_HOT __attribute__((hot))
#else
#define CP_GNU_LIKELY(value) (value)
#define CP_GNU_UNLIKELY(value) (value)
#define CP_GNU_ALWAYS_INLINE inline
#define CP_GNU_HOT
#endif

namespace cp::gnu {
inline constexpr bool extensions =
#if defined(__GNUC__) || defined(__clang__)
    true;
#else
    false;
#endif
}
