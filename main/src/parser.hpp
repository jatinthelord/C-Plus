#pragma once
#include "ast.hpp"
#include <vector>

namespace csp {
class DiagnosticEngine;

class Parser {
  const std::vector<Token> &t_;
  std::size_t p_ = 0;
  DiagnosticEngine *diagnostics_ = nullptr;
  const Token &peek(std::size_t n = 0) const;
  bool accept(const std::string &);
  const Token &require(const std::string &);
  std::string collect_until(const std::string &end, bool consume = true);
  std::unique_ptr<Node> expression(int minimum_precedence = 1);
  std::unique_ptr<Node> prefix();
  std::unique_ptr<Node> postfix(std::unique_ptr<Node> left);
  std::unique_ptr<Node> expression_until(const std::string &delimiter);
  static int precedence(const std::string &op);
  static bool right_associative(const std::string &op);
  std::unique_ptr<Node> block();
  std::unique_ptr<Node> statement();
  std::unique_ptr<Node> top_level();
  std::unique_ptr<Node> parse_if();
  std::unique_ptr<Node> parse_while();
  std::unique_ptr<Node> parse_do_while();
  std::unique_ptr<Node> parse_for();
  std::unique_ptr<Node> parse_switch();
  std::unique_ptr<Node> parse_match();
  std::unique_ptr<Node> parse_case();
  std::unique_ptr<Node> parse_try();
  std::unique_ptr<Node> parse_throw();
  std::unique_ptr<Node> parse_return(NodeKind kind = NodeKind::Return);
  std::unique_ptr<Node> parse_static_assert();
  std::unique_ptr<Node> parse_asm();
  std::unique_ptr<Node> parse_namespace();
  std::unique_ptr<Node> parse_record(NodeKind kind);
  std::unique_ptr<Node> parse_enum();
  std::unique_ptr<Node> parse_using();
  std::unique_ptr<Node> parse_template();
  std::vector<std::unique_ptr<Node>> parse_parameter_list();
  std::unique_ptr<Node> parse_external_declaration();
  bool token_starts_type(const Token &token) const;
  bool token_starts_expression(const Token &token) const;
  bool at_end() const;
  void synchronize_statement();
  std::string collect_balanced(const std::string &open,
                               const std::string &close);

public:
  explicit Parser(const std::vector<Token> &tokens) : t_(tokens) {}
  std::unique_ptr<Node> parse();
  std::unique_ptr<Node> parse_recovering(DiagnosticEngine &diagnostics);
};
} // namespace csp
