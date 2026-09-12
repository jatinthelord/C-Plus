#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

// bugemoan is C+'s deliberately explicit nightmare-level pointer API. It is
// for allocators, kernels, drivers, FFI, and runtimes—not ordinary ownership.
namespace bugemoan {

class fault : public std::runtime_error { public: using std::runtime_error::runtime_error; };

class key {
  explicit key(int) noexcept {}
  friend key enter_nightmare() noexcept;
};

[[nodiscard]] inline key enter_nightmare() noexcept { return key{0}; }

class void_pointer {
  void *address_ = nullptr;
  std::size_t bytes_ = 0;
  std::size_t alignment_ = 1;
  bool owning_ = false;

  void dispose() noexcept {
    if (owning_ && address_)
      ::operator delete(address_, std::align_val_t{alignment_});
  }

public:
  void_pointer() noexcept = default;
  void_pointer(const void_pointer &) = delete;
  void_pointer &operator=(const void_pointer &) = delete;
  void_pointer(void_pointer &&other) noexcept
      : address_(std::exchange(other.address_, nullptr)),
        bytes_(std::exchange(other.bytes_, 0)), alignment_(other.alignment_),
        owning_(std::exchange(other.owning_, false)) {}
  void_pointer &operator=(void_pointer &&other) noexcept {
    if (this != &other) {
      dispose();
      address_ = std::exchange(other.address_, nullptr);
      bytes_ = std::exchange(other.bytes_, 0);
      alignment_ = other.alignment_;
      owning_ = std::exchange(other.owning_, false);
    }
    return *this;
  }
  ~void_pointer() { dispose(); }

  [[nodiscard]] static void_pointer allocate(key, std::size_t bytes,
                                             std::size_t alignment = alignof(std::max_align_t)) {
    if (!bytes || !alignment || (alignment & (alignment - 1)))
      throw fault("bugemoan allocation requires nonzero bytes and power-of-two alignment");
    void_pointer result;
    result.address_ = ::operator new(bytes, std::align_val_t{alignment});
    result.bytes_ = bytes; result.alignment_ = alignment; result.owning_ = true;
    return result;
  }

  template <class T> [[nodiscard]] static void_pointer borrow(key, T *pointer,
                                                               std::size_t count = 1) {
    if (!pointer && count)
      throw fault("bugemoan cannot borrow a null non-empty region");
    void_pointer result; result.address_ = pointer; result.bytes_ = sizeof(T) * count;
    result.alignment_ = alignof(T); return result;
  }

  template <class T> [[nodiscard]] T *recover(key) const {
    static_assert(!std::is_void_v<T>, "recover a concrete type, not void");
    if (!address_)
      throw fault("bugemoan null recovery");
    if (bytes_ < sizeof(T) || reinterpret_cast<std::uintptr_t>(address_) % alignof(T))
      throw fault("bugemoan recovery violates size or alignment");
    return static_cast<T *>(address_);
  }

  [[nodiscard]] void_pointer offset(key access, std::size_t bytes) const {
    if (bytes > bytes_)
      throw fault("bugemoan pointer arithmetic escaped its region");
    return borrow_bytes(access, static_cast<std::byte *>(address_) + bytes,
                        bytes_ - bytes, alignment_);
  }

  [[nodiscard]] void *raw(key) const noexcept { return address_; }
  [[nodiscard]] std::size_t size_bytes() const noexcept { return bytes_; }
  [[nodiscard]] bool owns_memory() const noexcept { return owning_; }
  explicit operator bool() const noexcept { return address_ != nullptr; }

  [[nodiscard]] void *release(key) noexcept {
    owning_ = false; bytes_ = 0; return std::exchange(address_, nullptr);
  }

private:
  static void_pointer borrow_bytes(key, void *pointer, std::size_t bytes,
                                   std::size_t alignment) noexcept {
    void_pointer result; result.address_ = pointer; result.bytes_ = bytes;
    result.alignment_ = alignment; return result;
  }
};

template <class T, class... Arguments>
[[nodiscard]] T *construct(key access, void_pointer &storage,
                           Arguments &&...arguments) {
  return ::new (storage.recover<T>(access)) T(std::forward<Arguments>(arguments)...);
}

template <class T> void destroy(key, T *object) noexcept {
  if (object)
    object->~T();
}

} // namespace bugemoan
