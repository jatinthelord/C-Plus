#pragma once
#include "token.hpp"
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace csp {
class CompileError : public std::runtime_error {
public:
  Location location;
  CompileError(Location at, const std::string &message)
      : std::runtime_error(message), location(std::move(at)) {}
};

enum class DiagnosticSeverity { Note, Warning, Error, Fatal };

struct Diagnostic {
  DiagnosticSeverity severity = DiagnosticSeverity::Error;
  Location location;
  std::string message;
  std::vector<std::string> notes;
};

inline const char *severity_name(DiagnosticSeverity severity) {
  switch (severity) {
  case DiagnosticSeverity::Note:
    return "note";
  case DiagnosticSeverity::Warning:
    return "warning";
  case DiagnosticSeverity::Error:
    return "error";
  case DiagnosticSeverity::Fatal:
    return "fatal error";
  }
  return "diagnostic";
}

inline bool diagnostic_colors_enabled(std::ostream &output) {
  if (std::getenv("NO_COLOR") || output.rdbuf() != std::cerr.rdbuf())
    return false;
#ifdef _WIN32
  return _isatty(_fileno(stderr)) != 0;
#else
  return isatty(fileno(stderr)) != 0;
#endif
}

inline const char *severity_color(DiagnosticSeverity severity) {
  switch (severity) {
  case DiagnosticSeverity::Note: return "\x1b[36m";
  case DiagnosticSeverity::Warning: return "\x1b[33m";
  case DiagnosticSeverity::Error:
  case DiagnosticSeverity::Fatal: return "\x1b[31;1m";
  }
  return "";
}

class DiagnosticEngine {
  std::vector<Diagnostic> diagnostics_;
  std::size_t errors_ = 0;
  std::size_t warnings_ = 0;
  std::size_t error_limit_ = 20;
  bool warnings_as_errors_ = false;

public:
  explicit DiagnosticEngine(std::size_t error_limit = 20)
      : error_limit_(error_limit) {}
  void set_warnings_as_errors(bool enabled) { warnings_as_errors_ = enabled; }
  Diagnostic &report(DiagnosticSeverity severity, Location location,
                     std::string message) {
    if (severity == DiagnosticSeverity::Warning && warnings_as_errors_)
      severity = DiagnosticSeverity::Error;
    if (severity == DiagnosticSeverity::Warning)
      ++warnings_;
    if (severity == DiagnosticSeverity::Error ||
        severity == DiagnosticSeverity::Fatal)
      ++errors_;
    diagnostics_.push_back(
        {severity, std::move(location), std::move(message), {}});
    return diagnostics_.back();
  }
  Diagnostic &error(Location location, std::string message) {
    return report(DiagnosticSeverity::Error, std::move(location),
                  std::move(message));
  }
  Diagnostic &warning(Location location, std::string message) {
    return report(DiagnosticSeverity::Warning, std::move(location),
                  std::move(message));
  }
  bool has_errors() const { return errors_ != 0; }
  bool reached_error_limit() const { return errors_ >= error_limit_; }
  std::size_t error_count() const { return errors_; }
  std::size_t warning_count() const { return warnings_; }
  const std::vector<Diagnostic> &diagnostics() const { return diagnostics_; }
  void print(std::ostream &output) const {
    const bool colored = diagnostic_colors_enabled(output);
    for (const auto &diagnostic : diagnostics_) {
      output << diagnostic.location.file << ':' << diagnostic.location.line
             << ':' << diagnostic.location.column << ": ";
      if (colored)
        output << severity_color(diagnostic.severity);
      output << severity_name(diagnostic.severity);
      if (colored)
        output << "\x1b[0m";
      output << ": " << diagnostic.message
             << '\n';
      for (const auto &note : diagnostic.notes) {
        output << "  ";
        if (colored)
          output << severity_color(DiagnosticSeverity::Note);
        output << "note";
        if (colored)
          output << "\x1b[0m";
        output << ": " << note << '\n';
      }
    }
  }
};
} // namespace csp
