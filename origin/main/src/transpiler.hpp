#pragma once
#include "ast.hpp"
#include "token.hpp"
#include <string>
#include <vector>

namespace csp {
class Transpiler {
  static std::string indent(unsigned depth);
  static std::string rust_type(const std::string &type);
  static std::string rust_literal(const std::string &literal, bool c_string);
  static std::string rust_expression(const Node &node);
  static void rust_statement(const Node &node, std::string &output,
                             unsigned depth);
  static void rust_function(const Node &node, std::string &output);

public:
  static std::string emit_c(const std::string &lowered,
                            const std::vector<Token> &tokens);
  static std::string emit_rust(const Node &program);
};
} // namespace csp
