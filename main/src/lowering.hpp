#pragma once
#include "token.hpp"
#include <string>
#include <vector>

namespace csp {
struct SourceEdit {
  std::size_t begin = 0;
  std::size_t end = 0;
  std::string replacement;
  std::string reason;
};

class Lowering {
  const std::string &source_;
  const std::vector<Token> &tokens_;
  std::vector<SourceEdit> edits_;

  void lower_runtime_include();
  void lower_goto_spelling();
  void lower_print_arguments();
  void lower_short_for_loops();
  void lower_array_initializers();
  void lower_bindings();
  void lower_optional_types();
  std::size_t matching(std::size_t start, const std::string &open,
                       const std::string &close) const;
  std::string source_between(std::size_t first, std::size_t last) const;
  void add_edit(std::size_t begin, std::size_t end, std::string replacement,
                std::string reason);
  static bool starts_expression(const Token &token);

public:
  Lowering(const std::string &source, const std::vector<Token> &tokens)
      : source_(source), tokens_(tokens) {}
  std::string run();
  const std::vector<SourceEdit> &edits() const { return edits_; }
};
} // namespace csp
