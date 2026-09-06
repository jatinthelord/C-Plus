#pragma once
#include "token.hpp"
#include <string_view>
#include <vector>

namespace csp {
class Lexer {
  std::string_view source_;
  std::string file_;
  std::size_t pos_ = 0;
  int line_ = 1;
  int column_ = 1;
  bool beginning_of_line_ = true;

  char peek(std::size_t n = 0) const;
  char take();
  Location location() const;
  bool eof() const;
  bool starts_with(std::string_view text) const;
  bool consume(std::string_view text);
  void skip_horizontal_space();
  void skip_whitespace();
  void skip_line_comment();
  void skip_block_comment();
  Token scan_preprocessor();
  Token scan_identifier();
  Token scan_number();
  Token scan_quoted_literal();
  Token scan_raw_string();
  Token scan_operator_or_punctuation();
  std::string scan_escape(Location start);
  static bool is_identifier_start(unsigned char value);
  static bool is_identifier_continue(unsigned char value);
  static bool is_keyword(std::string_view text);
  static bool is_digit_for_base(char value, int base);
  static bool is_integer_suffix(char value);
  static bool is_float_suffix(char value);
  static bool is_punctuation(char value);

public:
  Lexer(std::string_view source, std::string file)
      : source_(source), file_(std::move(file)) {}
  std::vector<Token> tokenize();
};
} // namespace csp
