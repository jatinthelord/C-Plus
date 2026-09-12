#pragma once
#include <atomic>

namespace cp {
using memory_order = std::memory_order;
inline constexpr auto relaxed = std::memory_order_relaxed;
inline constexpr auto acquire = std::memory_order_acquire;
inline constexpr auto release = std::memory_order_release;
inline constexpr auto acquire_release = std::memory_order_acq_rel;
inline constexpr auto sequential = std::memory_order_seq_cst;
template <class T> using atomic = std::atomic<T>;
using atomic_flag = std::atomic_flag;
inline void thread_fence(memory_order order = sequential) {
  std::atomic_thread_fence(order);
}
} // namespace cp
