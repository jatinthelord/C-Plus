#pragma once
#include <cp/hash.h>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace cp {
struct Uuid {
  std::uint64_t hi = 0;
  std::uint64_t lo = 0;
};

inline std::uint64_t uuid_mix(std::uint64_t value) {
  value += 0x9e3779b97f4a7c15ull;
  value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ull;
  value = (value ^ (value >> 27)) * 0x94d049bb133111ebull;
  return value ^ (value >> 31);
}

inline Uuid uuid_from_name(std::string_view name) {
  const std::uint64_t a = fnv1a64(name);
  return Uuid{a, uuid_mix(a)};
}

inline std::string uuid_string(Uuid id) {
  char buffer[37];
  std::snprintf(buffer, sizeof(buffer), "%08x-%04x-%04x-%04x-%012llx",
                static_cast<unsigned>(id.hi >> 32),
                static_cast<unsigned>((id.hi >> 16) & 0xffff),
                static_cast<unsigned>(id.hi & 0xffff),
                static_cast<unsigned>(id.lo >> 48),
                static_cast<unsigned long long>(id.lo & 0xffffffffffffull));
  return buffer;
}
} // namespace cp
