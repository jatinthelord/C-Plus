#pragma once
#include <cstddef>
#include <type_traits>

#define CSP_EXTERN_C extern "C"
#define CSP_ALIGN(bytes) alignas(bytes)
#if defined(__GNUC__) || defined(__clang__)
#define CSP_PACKED __attribute__((packed))
#define CSP_FORCE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define CSP_PACKED
#define CSP_FORCE_INLINE __forceinline
#else
#define CSP_PACKED
#define CSP_FORCE_INLINE inline
#endif

namespace cp::abi {
template <class T>
inline constexpr bool c_compatible_layout =
    std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T>;

template <class T>
consteval void require_c_layout() {
  static_assert(c_compatible_layout<T>,
                "C ABI values must be standard-layout and trivially copyable");
}

template <class T>
inline constexpr std::size_t size = sizeof(T);
template <class T>
inline constexpr std::size_t alignment = alignof(T);
} // namespace cp::abi
