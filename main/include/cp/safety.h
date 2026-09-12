#pragma once

#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace sg {

class violation : public std::logic_error {
public:
  using std::logic_error::logic_error;
};

inline void require(bool condition, const char *message = "safety guard failed") {
  if (!condition)
    throw violation(message);
}

template <class T> class not_null {
  static_assert(std::is_pointer_v<T>, "sg::not_null requires a pointer type");
  T pointer_;

public:
  explicit not_null(T pointer) : pointer_(pointer) {
    require(pointer != nullptr, "null pointer passed to sg::not_null");
  }
  T get() const noexcept { return pointer_; }
  operator T() const noexcept { return pointer_; }
  decltype(auto) operator*() const { return *pointer_; }
  T operator->() const noexcept { return pointer_; }
};

template <class To, class From> To checked_cast(From value) {
  static_assert(std::is_arithmetic_v<To> && std::is_arithmetic_v<From>);
  const To converted = static_cast<To>(value);
  if (static_cast<From>(converted) != value)
    throw violation("numeric conversion loses information");
  if constexpr (std::is_signed_v<From> != std::is_signed_v<To>) {
    if ((value < 0) != (converted < 0))
      throw violation("numeric conversion changes sign");
  }
  return converted;
}

template <class Function> class scope_guard {
  Function function_;
  bool active_ = true;

public:
  explicit scope_guard(Function function) : function_(std::move(function)) {}
  scope_guard(const scope_guard &) = delete;
  scope_guard &operator=(const scope_guard &) = delete;
  scope_guard(scope_guard &&other) noexcept
      : function_(std::move(other.function_)), active_(other.active_) {
    other.active_ = false;
  }
  ~scope_guard() noexcept(noexcept(function_())) { if (active_) function_(); }
  void dismiss() noexcept { active_ = false; }
};

template <class Function> scope_guard<Function> finally(Function function) {
  return scope_guard<Function>(std::move(function));
}

} // namespace sg

namespace bg {

class bounds_error : public std::out_of_range {
public:
  using std::out_of_range::out_of_range;
};

template <class T> class buffer {
  std::span<T> values_;

public:
  explicit buffer(std::span<T> values) : values_(values) {}
  template <std::size_t Size>
  buffer(T (&values)[Size]) : values_(values, Size) {}
  buffer(T *data, std::size_t size) : values_(data, size) {
    if (!data && size != 0)
      throw bounds_error("null data passed to non-empty bg::buffer");
  }
  T &at(std::size_t index) const {
    if (index >= values_.size())
      throw bounds_error("bg::buffer index out of bounds");
    return values_[index];
  }
  T *data() const noexcept { return values_.data(); }
  std::size_t size() const noexcept { return values_.size(); }
  auto begin() const noexcept { return values_.begin(); }
  auto end() const noexcept { return values_.end(); }
};

template <class T, std::size_t Size> buffer(T (&)[Size]) -> buffer<T>;

} // namespace bg
