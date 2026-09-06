/*
 * C+ Process Library
 * Copyright (c) 2026 CSP Foundation
 * Licensed under the MIT License.
 */
#pragma once

#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cp::process {

struct result {
  int exit_code = -1;
  std::string output;
  constexpr bool successful() const noexcept { return exit_code == 0; }
};

inline result run(std::string_view command) {
  result value;
  value.exit_code = std::system(std::string(command).c_str());
  return value;
}

inline int exit_code(int status) noexcept {
#if defined(_WIN32)
  return status;
#else
  return (status >= 0 && status <= 255) ? status : -1;
#endif
}

class command {
public:
  explicit command(std::string executable) : executable_(std::move(executable)) {}
  command &argument(std::string value) { arguments_.push_back(std::move(value)); return *this; }
  std::string render() const {
    std::string output = executable_;
    for (const auto &argument : arguments_) { output += " \""; output += argument; output += '\"'; }
    return output;
  }
  result execute() const { return run(render()); }

private:
  std::string executable_;
  std::vector<std::string> arguments_;
};

} // namespace cp::process
