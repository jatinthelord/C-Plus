#pragma once
#include <atomic>
#include <thread>
#include <utility>

namespace cp {
template <class Fn> void spawn(Fn &&work) {
  std::thread(std::forward<Fn>(work)).detach();
}

template <class Fn> void join_run(Fn &&work) {
  std::thread worker(std::forward<Fn>(work));
  worker.join();
}

inline unsigned hardware_threads() {
  const unsigned count = std::thread::hardware_concurrency();
  return count == 0 ? 1 : count;
}

class SpinLock {
  std::atomic_flag flag = ATOMIC_FLAG_INIT;

public:
  void lock() {
    while (flag.test_and_set(std::memory_order_acquire))
      std::this_thread::yield();
  }
  void unlock() { flag.clear(std::memory_order_release); }
};
} // namespace cp
