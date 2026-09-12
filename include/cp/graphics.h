#pragma once
#include <algorithm>
#include <cstdint>

namespace cp {
struct Color {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
  std::uint8_t a = 255;
};

struct Rect {
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
};

inline Color blend(Color dst, Color src) {
  const int alpha = src.a;
  const int inv = 255 - alpha;
  dst.r = static_cast<std::uint8_t>((src.r * alpha + dst.r * inv) / 255);
  dst.g = static_cast<std::uint8_t>((src.g * alpha + dst.g * inv) / 255);
  dst.b = static_cast<std::uint8_t>((src.b * alpha + dst.b * inv) / 255);
  dst.a = static_cast<std::uint8_t>(std::min(255, dst.a + src.a));
  return dst;
}

inline bool contains(Rect rect, int px, int py) {
  return px >= rect.x && py >= rect.y && px < rect.x + rect.w &&
         py < rect.y + rect.h;
}
} // namespace cp
