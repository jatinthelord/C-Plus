#pragma once
#include <cctype>
#include <string>
#include <string_view>

namespace cp {
inline std::string json_escape(std::string_view text) {
  std::string out;
  out.push_back('"');
  for (unsigned char ch : text) {
    switch (ch) {
    case '"':
      out += "\\\"";
      break;
    case '\\':
      out += "\\\\";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      if (ch < 0x20) {
        out += "\\u00";
        const char *hex = "0123456789abcdef";
        out.push_back(hex[ch >> 4]);
        out.push_back(hex[ch & 15]);
      } else
        out.push_back(static_cast<char>(ch));
    }
  }
  out.push_back('"');
  return out;
}

inline bool json_is_true(std::string_view text) { return text == "true"; }
inline bool json_is_null(std::string_view text) { return text == "null"; }
} // namespace cp
