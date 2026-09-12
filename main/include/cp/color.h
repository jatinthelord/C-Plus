#pragma once

#include <array>
#include <cstdlib>
#include <ostream>
#include <string>
#include <string_view>

namespace cp::color {

enum class basic : unsigned char {
  black = 0, red, green, yellow, blue, magenta, cyan, white,
  bright_black, bright_red, bright_green, bright_yellow,
  bright_blue, bright_magenta, bright_cyan, bright_white
};

enum class decoration : unsigned char {
  reset = 0, bold = 1, faint = 2, italic = 3, underline = 4,
  inverse = 7, hidden = 8, strike = 9
};

struct rgb {
  unsigned char red{};
  unsigned char green{};
  unsigned char blue{};
};

inline bool enabled() noexcept {
  if (std::getenv("NO_COLOR"))
    return false;
  const char *term = std::getenv("TERM");
  return !term || std::string_view(term) != "dumb";
}

inline std::string escape(int code) {
  return "\x1b[" + std::to_string(code) + 'm';
}

inline std::string foreground(basic value) {
  const int raw = static_cast<int>(value);
  return escape(raw < 8 ? 30 + raw : 90 + raw - 8);
}

inline std::string background(basic value) {
  const int raw = static_cast<int>(value);
  return escape(raw < 8 ? 40 + raw : 100 + raw - 8);
}

inline std::string foreground(rgb value) {
  return "\x1b[38;2;" + std::to_string(value.red) + ';' +
         std::to_string(value.green) + ';' + std::to_string(value.blue) + 'm';
}

inline std::string background(rgb value) {
  return "\x1b[48;2;" + std::to_string(value.red) + ';' +
         std::to_string(value.green) + ';' + std::to_string(value.blue) + 'm';
}

inline std::string decorate(std::string_view text, basic foreground_color,
                            decoration effect = decoration::reset) {
  if (!enabled())
    return std::string(text);
  std::string output;
  if (effect != decoration::reset)
    output += escape(static_cast<int>(effect));
  output += foreground(foreground_color);
  output += text;
  output += escape(0);
  return output;
}

inline std::string decorate(std::string_view text, rgb foreground_color) {
  return enabled() ? foreground(foreground_color) + std::string(text) + escape(0)
                   : std::string(text);
}

struct styled_text {
  std::string_view text;
  basic foreground_color = basic::white;
  decoration effect = decoration::reset;
};

inline std::ostream &operator<<(std::ostream &stream, const styled_text &value) {
  return stream << decorate(value.text, value.foreground_color, value.effect);
}

namespace palette {
inline constexpr rgb csp_blue{23, 147, 209};
inline constexpr rgb success{35, 209, 139};
inline constexpr rgb warning{255, 184, 77};
inline constexpr rgb error{244, 71, 71};
} // namespace palette

} // namespace cp::color
