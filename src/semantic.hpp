#pragma once
#include "ast.hpp"
#include "diagnostic.hpp"
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace csp {
class SemanticAnalyzer {
  DiagnosticEngine &diagnostics_;
  std::vector<std::unordered_map<std::string, std::vector<const Node *>>>
      scopes_;
  std::unordered_map<std::string, Location> labels_;
  std::vector<std::pair<std::string, Location>> gotos_;
  unsigned function_depth_ = 0;
  unsigned loop_depth_ = 0;
  unsigned switch_depth_ = 0;
  std::vector<bool> switch_has_default_;
  std::vector<std::unordered_map<std::string, Location>> switch_cases_;
  std::unordered_map<std::string, std::vector<std::string>> enum_members_;
  std::vector<std::unordered_map<std::string, std::string>> variable_types_;
  std::vector<std::unordered_set<std::string>> optional_variables_;
  std::vector<std::unordered_set<std::string>> immutable_variables_;
  bool optional_visible(const std::string &name) const;
  bool immutable_visible(const std::string &name) const;
  std::unordered_map<std::string, const Node *> ctfe_functions_;
  std::vector<std::unordered_map<std::string, long long>> ctfe_bindings_;
  unsigned ctfe_depth_ = 0;
  void visit(const Node &node);
  void visit_children(const Node &node);
  void enter_scope();
  void leave_scope();
  void declare_name(const std::string &name, const Node &node,
                    bool allow_overload = false);
  void visit_callable(const Node &node);
  void validate_gotos();
  static std::string declaration_name(const std::string &text);
  static std::string function_signature(const Node &node);
  static bool has_body(const Node &node);
  static std::optional<std::string> constant_case_key(const Node &node);
  std::optional<long long> evaluate_constant(const Node &node);
  std::optional<long long>
  evaluate_ctfe_function(const Node &function,
                         const std::vector<long long> &arguments);
  static std::string declaration_type(const std::string &text);
  static std::string normalized_name(std::string text);
  std::string lookup_type(const std::string &name) const;
  bool name_visible(const std::string &name) const;
  std::optional<std::string> closest_name(const std::string &name) const;
  void
  check_exhaustive_match(const Node &node,
                         const std::unordered_map<std::string, Location> &cases,
                         bool has_default);

public:
  explicit SemanticAnalyzer(DiagnosticEngine &diagnostics)
      : diagnostics_(diagnostics) {}
  bool analyze(const Node &program);
};
} // namespace csp
