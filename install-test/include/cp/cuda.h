#pragma once
#include <cstddef>
#include <stdexcept>
#include <utility>

#if defined(__CUDACC__) || defined(CSP_CUDA)
#include <cuda_runtime.h>
namespace cp::cuda {
class error : public std::runtime_error {
  cudaError_t code_;

public:
  explicit error(cudaError_t code)
      : std::runtime_error(cudaGetErrorString(code)), code_(code) {}
  cudaError_t code() const noexcept { return code_; }
};

inline void check(cudaError_t status) {
  if (status != cudaSuccess)
    throw error(status);
}
inline void synchronize() { check(cudaDeviceSynchronize()); }
inline void check_last_launch() { check(cudaGetLastError()); }

template <class T> class device_buffer {
  T *data_ = nullptr;
  std::size_t size_ = 0;

public:
  device_buffer() = default;
  explicit device_buffer(std::size_t size) : size_(size) {
    check(cudaMalloc(reinterpret_cast<void **>(&data_), size * sizeof(T)));
  }
  device_buffer(const device_buffer &) = delete;
  device_buffer &operator=(const device_buffer &) = delete;
  device_buffer(device_buffer &&other) noexcept
      : data_(std::exchange(other.data_, nullptr)),
        size_(std::exchange(other.size_, 0)) {}
  device_buffer &operator=(device_buffer &&other) noexcept {
    if (this != &other) {
      if (data_)
        cudaFree(data_);
      data_ = std::exchange(other.data_, nullptr);
      size_ = std::exchange(other.size_, 0);
    }
    return *this;
  }
  ~device_buffer() {
    if (data_)
      cudaFree(data_);
  }
  T *data() noexcept { return data_; }
  const T *data() const noexcept { return data_; }
  std::size_t size() const noexcept { return size_; }
  void copy_from_host(const T *source, std::size_t count) {
    if (count > size_)
      throw std::out_of_range("device buffer upload exceeds allocation");
    check(cudaMemcpy(data_, source, count * sizeof(T), cudaMemcpyHostToDevice));
  }
  void copy_to_host(T *destination, std::size_t count) const {
    if (count > size_)
      throw std::out_of_range("device buffer download exceeds allocation");
    check(cudaMemcpy(destination, data_, count * sizeof(T),
                     cudaMemcpyDeviceToHost));
  }
};
} // namespace cp::cuda
#endif
