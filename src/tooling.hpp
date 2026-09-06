#pragma once
#include "ast.hpp"
#include "token.hpp"
#include <string>
#include <vector>

namespace csp {
struct LintIssue {
  Location location;
  std::string rule;
  std::string message;
  bool error = false;
};
std::string format_source(const std::string &source);
std::vector<LintIssue> lint_source(const std::string &source,
                                   const std::vector<Token> &tokens,
                                   const std::string &filename);
std::string reflection_json(const Node &program, const std::string &filename);
} // namespace csp
