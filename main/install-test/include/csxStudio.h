#pragma once

#include <cp/stdlib.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace csx::studio {

enum class target { native, c, cpp, rust, llvm, assembly, cuda };
enum class optimization { debug, balanced, speed, maximum };

struct diagnostic {
  std::string file;
  std::size_t line = 0;
  std::size_t column = 0;
  std::string message;
  bool error = true;
};

struct project {
  std::string name;
  std::string entry = "main.csp";
  std::string output;
  target build_target = target::native;
  optimization optimize = optimization::balanced;
  std::vector<std::string> include_paths;
  std::vector<std::string> definitions;
  std::vector<std::string> libraries;
};

inline std::string_view target_flag(target value) noexcept {
  switch (value) {
  case target::c: return "--emit-c";
  case target::rust: return "--emit-rust";
  case target::llvm: return "--emit-llvm";
  case target::assembly: return "-S";
  case target::cuda: return "--cuda";
  case target::native: return "--native";
  case target::cpp: return "--emit-cpp";
  }
  return "--native";
}

inline std::string optimization_flag(optimization value) {
  switch (value) {
  case optimization::debug: return "-O0";
  case optimization::balanced: return "-O2";
  case optimization::speed: return "-O3";
  case optimization::maximum: return "-O3 --lto --native";
  }
  return "-O2";
}

inline std::vector<std::string> command(const project &value,
                                        std::string compiler = "cspc") {
  std::vector<std::string> result{std::move(compiler), value.entry,
                                  std::string(target_flag(value.build_target)),
                                  optimization_flag(value.optimize)};
  if (!value.output.empty()) {
    result.push_back("-o");
    result.push_back(value.output);
  }
  for (const auto &path : value.include_paths)
    result.push_back("-I" + path);
  for (const auto &definition : value.definitions)
    result.push_back("-D" + definition);
  for (const auto &library : value.libraries)
    result.push_back("-l" + library);
  return result;
}

} // namespace csx::studio
