#pragma once

#include <array>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace cp {

template <class Signature>
using function_pointer = Signature *;

template <class Enum>
class bit_flags {
  static_assert(std::is_enum_v<Enum>, "bit_flags requires an enum type");
  using integer = std::underlying_type_t<Enum>;
  integer bits_ = 0;

public:
  constexpr bit_flags() noexcept = default;
  constexpr bit_flags(Enum value) noexcept : bits_(static_cast<integer>(value)) {}
  constexpr explicit bit_flags(integer value) noexcept : bits_(value) {}

  constexpr integer bits() const noexcept { return bits_; }
  constexpr bool empty() const noexcept { return bits_ == 0; }
  constexpr bool has(Enum value) const noexcept {
    const integer mask = static_cast<integer>(value);
    return (bits_ & mask) == mask;
  }
  constexpr bit_flags &set(Enum value) noexcept {
    bits_ |= static_cast<integer>(value);
    return *this;
  }
  constexpr bit_flags &clear(Enum value) noexcept {
    bits_ &= ~static_cast<integer>(value);
    return *this;
  }
  constexpr bit_flags &toggle(Enum value) noexcept {
    bits_ ^= static_cast<integer>(value);
    return *this;
  }
  friend constexpr bit_flags operator|(bit_flags left, Enum right) noexcept {
    return left.set(right);
  }
  friend constexpr bit_flags operator&(bit_flags left, Enum right) noexcept {
    return bit_flags(left.bits_ & static_cast<integer>(right));
  }
};

template <class T, std::size_t Size>
using lookup_table = std::array<T, Size>;

template <class T, std::size_t Size, class Generator>
consteval lookup_table<T, Size> make_lookup_table(Generator generator) {
  lookup_table<T, Size> result{};
  for (std::size_t index = 0; index < Size; ++index)
    result[index] = static_cast<T>(generator(index));
  return result;
}

template <std::size_t Capacity>
class char_array {
  static_assert(Capacity > 0, "char_array needs space for a terminator");
  std::array<char, Capacity> data_{};
  std::size_t size_ = 0;

public:
  constexpr char_array() noexcept = default;
  constexpr char_array(std::string_view value) { assign(value); }
  constexpr void assign(std::string_view value) {
    if (value.size() >= Capacity)
      throw std::length_error("text does not fit in cp::char_array");
    size_ = value.size();
    for (std::size_t index = 0; index < size_; ++index)
      data_[index] = value[index];
    data_[size_] = '\0';
  }
  constexpr const char *c_str() const noexcept { return data_.data(); }
  constexpr char *data() noexcept { return data_.data(); }
  constexpr const char *data() const noexcept { return data_.data(); }
  constexpr std::size_t size() const noexcept { return size_; }
  constexpr std::size_t capacity() const noexcept { return Capacity - 1; }
  constexpr std::string_view view() const noexcept { return {data_.data(), size_}; }
  constexpr char &operator[](std::size_t index) noexcept { return data_[index]; }
  constexpr char operator[](std::size_t index) const noexcept { return data_[index]; }
};

template <std::size_t Size>
struct string_literal {
  char value[Size]{};
  consteval string_literal(const char (&text)[Size]) {
    for (std::size_t index = 0; index < Size; ++index) value[index] = text[index];
  }
  constexpr std::size_t size() const noexcept { return Size - 1; }
  constexpr const char *data() const noexcept { return value; }
  constexpr std::string_view view() const noexcept { return {value, Size - 1}; }
};
template <std::size_t Size>
string_literal(const char (&)[Size]) -> string_literal<Size>;

enum class union_tag : std::uint8_t { first, second };

template <class First, class Second>
class tagged_union {
  union storage {
    First first;
    Second second;
    constexpr storage() noexcept {}
    constexpr ~storage() noexcept {}
  } value_;
  union_tag tag_;

  explicit constexpr tagged_union(union_tag tag) : tag_(tag) {}
  constexpr void destroy() noexcept {
    if (tag_ == union_tag::first) value_.first.~First();
    else value_.second.~Second();
  }

public:
  static constexpr tagged_union first(First value) {
    tagged_union result(union_tag::first);
    std::construct_at(&result.value_.first, std::move(value));
    return result;
  }
  static constexpr tagged_union second(Second value) {
    tagged_union result(union_tag::second);
    std::construct_at(&result.value_.second, std::move(value));
    return result;
  }
  constexpr tagged_union(const tagged_union &other) : tag_(other.tag_) {
    if (tag_ == union_tag::first) std::construct_at(&value_.first, other.value_.first);
    else std::construct_at(&value_.second, other.value_.second);
  }
  constexpr tagged_union(tagged_union &&other) noexcept(
      std::is_nothrow_move_constructible_v<First> &&
      std::is_nothrow_move_constructible_v<Second>) : tag_(other.tag_) {
    if (tag_ == union_tag::first) std::construct_at(&value_.first, std::move(other.value_.first));
    else std::construct_at(&value_.second, std::move(other.value_.second));
  }
  constexpr ~tagged_union() { destroy(); }
  constexpr union_tag tag() const noexcept { return tag_; }
  constexpr bool is_first() const noexcept { return tag_ == union_tag::first; }
  constexpr bool is_second() const noexcept { return tag_ == union_tag::second; }
  constexpr First &first() {
    if (!is_first()) throw std::logic_error("tagged_union does not hold first");
    return value_.first;
  }
  constexpr const First &first() const {
    if (!is_first()) throw std::logic_error("tagged_union does not hold first");
    return value_.first;
  }
  constexpr Second &second() {
    if (!is_second()) throw std::logic_error("tagged_union does not hold second");
    return value_.second;
  }
  constexpr const Second &second() const {
    if (!is_second()) throw std::logic_error("tagged_union does not hold second");
    return value_.second;
  }
};

namespace literals {
consteval std::uint64_t operator""_KiB(unsigned long long value) { return value * 1024ULL; }
consteval std::uint64_t operator""_MiB(unsigned long long value) { return value * 1024ULL * 1024ULL; }
consteval std::uint64_t operator""_GiB(unsigned long long value) { return value * 1024ULL * 1024ULL * 1024ULL; }
} // namespace literals

} // namespace cp

#define CSP_VA_LIST(name) va_list name
#define CSP_VA_START(list, last) va_start(list, last)
#define CSP_VA_ARG(list, type) va_arg(list, type)
#define CSP_VA_END(list) va_end(list)
#define CSP_BITFIELD(type, name, width) type name : width
