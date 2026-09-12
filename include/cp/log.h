#pragma once
#include <cstdio>
#include <string_view>

namespace cp {
enum class LogLevel { Debug, Info, Warn, Error };

inline const char *log_name(LogLevel level) {
  switch (level) {
  case LogLevel::Debug:
    return "debug";
  case LogLevel::Info:
    return "info";
  case LogLevel::Warn:
    return "warn";
  case LogLevel::Error:
    return "error";
  }
  return "info";
}

inline void log(LogLevel level, std::string_view message) {
  std::fprintf(stderr, "[%s] %.*s\n", log_name(level),
               static_cast<int>(message.size()), message.data());
}

inline void info(std::string_view message) { log(LogLevel::Info, message); }
inline void warn(std::string_view message) { log(LogLevel::Warn, message); }
inline void error(std::string_view message) { log(LogLevel::Error, message); }
} // namespace cp
