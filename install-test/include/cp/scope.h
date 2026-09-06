#pragma once
#include <utility>

namespace cp {
template <class Function> class scope_exit {
  Function function_;
  bool active_ = true;

public:
  explicit scope_exit(Function function) : function_(std::move(function)) {}
  scope_exit(const scope_exit &) = delete;
  scope_exit &operator=(const scope_exit &) = delete;
  scope_exit(scope_exit &&other) noexcept
      : function_(std::move(other.function_)), active_(other.active_) {
    other.active_ = false;
  }
  ~scope_exit() {
    if (active_)
      function_();
  }
  void release() noexcept { active_ = false; }
};

template <class Function> scope_exit<Function> defer(Function function) {
  return scope_exit<Function>(std::move(function));
}
} // namespace cp
