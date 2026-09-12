/*
 * C+ Time Library
 * Copyright (c) 2026 CSP Foundation
 * Licensed under the MIT License.
 */
#pragma once

#include <chrono>
#include <cstdint>
#include <thread>
#include <utility>

namespace cp::time {

using clock = std::chrono::steady_clock;
using instant = clock::time_point;
using nanoseconds = std::chrono::nanoseconds;
using microseconds = std::chrono::microseconds;
using milliseconds = std::chrono::milliseconds;
using seconds = std::chrono::seconds;

inline instant now() noexcept { return clock::now(); }
inline std::int64_t elapsed_nanoseconds(instant start, instant end) noexcept { return std::chrono::duration_cast<nanoseconds>(end - start).count(); }
inline void sleep(milliseconds duration) { std::this_thread::sleep_for(duration); }

class stopwatch {
public:
  stopwatch() noexcept : started_(now()) {}
  void restart() noexcept { started_ = now(); }
  nanoseconds elapsed() const noexcept { return std::chrono::duration_cast<nanoseconds>(now() - started_); }
  std::int64_t elapsed_count() const noexcept { return elapsed().count(); }

private:
  instant started_;
};

template <class Function> auto measure(Function function) {
  stopwatch timer;
  auto value = function();
  return std::pair{value, timer.elapsed()};
}

} // namespace cp::time
