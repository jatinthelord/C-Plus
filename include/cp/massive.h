#pragma once
#include <cp/hash.h>
#include <cp/string.h>
#include <cstdint>
#include <string>
#include <string_view>

namespace cp {
struct Module {
  std::string_view name;
  std::string_view purpose;
};

constexpr Module language_surface[] = {
    {"lexer", "UTF-8 tokens, nested comments, digit separators"},
    {"parser", "C-family declarations with match, hopa, and recovery"},
    {"semantic", "overloads, exhaustiveness, optional safety"},
    {"thir", "typed HIR before CFG lowering"},
    {"mir", "verified control-flow IR"},
    {"cpasm", "native assembler frontend (.cpsm)"},
    {"stdlib", "containers, net, GPU, GUI, crypto, and I/O"},
};

inline std::uint32_t surface_fingerprint() {
  std::uint32_t hash = 2166136261u;
  for (const Module &module : language_surface)
    hash = fnv1a32(module.name) ^ rotl32(hash, 5);
  return hash;
}

inline std::string describe_surface() {
  std::string text = "C+ language surface\n";
  for (const Module &module : language_surface) {
    text += "- ";
    text += module.name;
    text += ": ";
    text += module.purpose;
    text += "\n";
  }
  return text;
}
} // namespace cp
