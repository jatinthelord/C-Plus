#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>

namespace cp {
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using usize = std::size_t;
using isize = std::ptrdiff_t;
#if defined(__SIZEOF_INT128__)
using i128 = __int128;
using u128 = unsigned __int128;
#endif
using unichar = char32_t;

template <class T>
inline constexpr T min_value = std::numeric_limits<T>::lowest();
template <class T> inline constexpr T max_value = std::numeric_limits<T>::max();
} // namespace cp
