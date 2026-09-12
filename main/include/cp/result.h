#pragma once
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace cp {
template <class E> struct unexpected {
  E error;
};

template <class T, class E> class result {
  std::variant<T, E> storage_;
  bool success_;

public:
  result(const T &value) : storage_(value), success_(true) {}
  result(T &&value) : storage_(std::move(value)), success_(true) {}
  result(unexpected<E> failure)
      : storage_(std::move(failure.error)), success_(false) {}

  bool has_value() const noexcept { return success_; }
  explicit operator bool() const noexcept { return success_; }

  T &value() {
    if (!success_)
      throw std::logic_error("accessed value of failed cp::result");
    return std::get<T>(storage_);
  }
  const T &value() const {
    if (!success_)
      throw std::logic_error("accessed value of failed cp::result");
    return std::get<T>(storage_);
  }
  E &error() {
    if (success_)
      throw std::logic_error("accessed error of successful cp::result");
    return std::get<E>(storage_);
  }
  const E &error() const {
    if (success_)
      throw std::logic_error("accessed error of successful cp::result");
    return std::get<E>(storage_);
  }
  T value_or(T fallback) const {
    return success_ ? std::get<T>(storage_) : std::move(fallback);
  }

  template <class Function>
  auto map(Function &&function) const
      -> result<std::invoke_result_t<Function, const T &>, E> {
    using Output = std::invoke_result_t<Function, const T &>;
    if (success_)
      return result<Output, E>(function(std::get<T>(storage_)));
    return unexpected<E>{std::get<E>(storage_)};
  }
};
} // namespace cp
