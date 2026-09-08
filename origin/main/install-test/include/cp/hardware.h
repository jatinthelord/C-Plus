#pragma once

#if defined(_MSC_VER)
#define CP_FORCE_INLINE __forceinline
#define CP_NO_INLINE __declspec(noinline)
#define CP_RESTRICT __restrict
#else
#define CP_FORCE_INLINE inline __attribute__((always_inline))
#define CP_NO_INLINE __attribute__((noinline))
#define CP_RESTRICT __restrict__
#endif

#if defined(__GNUC__) || defined(__clang__)
#define CP_LIKELY(value) __builtin_expect(!!(value), 1)
#define CP_UNLIKELY(value) __builtin_expect(!!(value), 0)
#define CP_PREFETCH_READ(address) __builtin_prefetch((address), 0, 3)
#define CP_PREFETCH_WRITE(address) __builtin_prefetch((address), 1, 3)
#define CP_UNREACHABLE() __builtin_unreachable()
#else
#define CP_LIKELY(value) (value)
#define CP_UNLIKELY(value) (value)
#define CP_PREFETCH_READ(address) ((void)0)
#define CP_PREFETCH_WRITE(address) ((void)0)
#define CP_UNREACHABLE() ((void)0)
#endif

#if defined(__CUDACC__)
#define CP_HOST __host__
#define CP_DEVICE __device__
#define CP_GLOBAL __global__
#define CP_HOST_DEVICE __host__ __device__
#else
#define CP_HOST
#define CP_DEVICE
#define CP_GLOBAL
#define CP_HOST_DEVICE
#endif
