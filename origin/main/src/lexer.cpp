#include "lexer.hpp"
#include "diagnostic.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <unordered_set>

namespace csp {
namespace {

// The lexical tables recognize both ISO C++ and the additional C+ vocabulary.

const std::unordered_set<std::string> &language_keywords() {
  static const std::unordered_set<std::string> values = {
      "alignas",
      "alignof",
      "and",
      "and_eq",
      "asm",
      "atomic_cancel",
      "atomic_commit",
      "atomic_noexcept",
      "auto",
      "bitand",
      "bitor",
      "bool",
      "bugemoan",
      "break",
      "case",
      "catch",
      "char",
      "char8_t",
      "char16_t",
      "char32_t",
      "class",
      "co_await",
      "co_return",
      "co_yield",
      "compl",
      "concept",
      "const",
      "consteval",
      "constexpr",
      "constinit",
      "const_cast",
      "continue",
      "contract_assert",
      "decltype",
      "default",
      "delete",
      "do",
      "double",
      "dynamic_cast",
      "else",
      "enum",
      "explicit",
      "export",
      "extern",
      "false",
      "float",
      "for",
      "friend",
      "goto",
      "Goto",
      "if",
      "inline",
      "int",
      "long",
      "let",
      "mut",
      "match",
      "mutable",
      "namespace",
      "new",
      "noexcept",
      "not",
      "not_eq",
      "nullptr",
      "operator",
      "or",
      "or_eq",
      "private",
      "protected",
      "public",
      "register",
      "reinterpret_cast",
      "requires",
      "return",
      "short",
      "signed",
      "sizeof",
      "static",
      "static_assert",
      "static_cast",
      "struct",
      "switch",
      "synchronized",
      "template",
      "this",
      "thread_local",
      "throw",
      "true",
      "try",
      "typedef",
      "typeid",
      "typename",
      "union",
      "unsigned",
      "using",
      "virtual",
      "void",
      "volatile",
      "wchar_t",
      "while",
      "xor",
      "xor_eq",
      "u8",
      "u16",
      "u32",
      "u64",
      "u128",
      "unichar",
      "size_t",
      "__hopa__",
      "__global__",
      "__host__",
      "__device__",
      "__shared__",
      "__constant__",
      "__managed__",
      "__restrict__",
      "__launch_bounds__",
      "__device_builtin__",
      "__cudart_builtin__",
      "__noinline__",
      "__forceinline__",
  };
  return values;
}

const std::array<std::string_view, 39> &operators() {
  static const std::array<std::string_view, 39> values = {
      "<=>", "<<=", ">>=", "...", "->*", "<<<", ">>>", "##", "::", ".*",
      "->",  "++",  "--",  "<<",  ">>",  "<=",  ">=",  "==", "!=", "&&",
      "||",  "+=",  "-=",  "*=",  "/=",  "%=",  "&=",  "|=", "^=", "(",
      ")",   "[",   "]",   "{",   "}",   ";",   ",",   ":",  ".",
  };
  return values;
}

bool is_hexadecimal_digit(char value) {
  return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f') ||
         (value >= 'A' && value <= 'F');
}

bool is_octal_digit(char value) { return value >= '0' && value <= '7'; }

bool is_binary_digit(char value) { return value == '0' || value == '1'; }

std::size_t utf8_sequence_length(unsigned char lead) {
  if ((lead & 0x80) == 0)
    return 1;
  if ((lead & 0xE0) == 0xC0)
    return 2;
  if ((lead & 0xF0) == 0xE0)
    return 3;
  if ((lead & 0xF8) == 0xF0)
    return 4;
  return 0;
}

bool is_utf8_continuation(unsigned char value) {
  return (value & 0xC0) == 0x80;
}

void validate_utf8(std::string_view text, const Location &start) {
  for (std::size_t index = 0; index < text.size();) {
    unsigned char lead = static_cast<unsigned char>(text[index]);
    std::size_t length = utf8_sequence_length(lead);
    if (length == 0)
      throw CompileError(start, "invalid UTF-8 leading byte in identifier");
    if (index + length > text.size())
      throw CompileError(start, "truncated UTF-8 sequence in identifier");
    for (std::size_t offset = 1; offset < length; ++offset)
      if (!is_utf8_continuation(
              static_cast<unsigned char>(text[index + offset])))
        throw CompileError(start,
                           "invalid UTF-8 continuation byte in identifier");
    if (length == 2 && lead < 0xC2)
      throw CompileError(start, "overlong UTF-8 sequence in identifier");
    if (length == 3) {
      unsigned char second = static_cast<unsigned char>(text[index + 1]);
      if (lead == 0xE0 && second < 0xA0)
        throw CompileError(start, "overlong UTF-8 sequence in identifier");
      if (lead == 0xED && second >= 0xA0)
        throw CompileError(start, "UTF-8 surrogate in identifier");
    }
    if (length == 4) {
      unsigned char second = static_cast<unsigned char>(text[index + 1]);
      if (lead == 0xF0 && second < 0x90)
        throw CompileError(start, "overlong UTF-8 sequence in identifier");
      if (lead > 0xF4 || (lead == 0xF4 && second > 0x8F))
        throw CompileError(start, "identifier code point exceeds U+10FFFF");
    }
    index += length;
  }
}

bool is_cuda_builtin(std::string_view text) {
  return text == "threadIdx" || text == "blockIdx" || text == "blockDim" ||
         text == "gridDim" || text == "warpSize" || text == "BlockGrid" ||
         text == "BlockDim";
}

bool is_csp_type(std::string_view text) {
  return text == "u8" || text == "u16" || text == "u32" || text == "u64" ||
         text == "u128" || text == "unichar";
}

} // namespace

char Lexer::peek(std::size_t offset) const {
  if (pos_ + offset >= source_.size())
    return '\0';
  return source_[pos_ + offset];
}

char Lexer::take() {
  char value = peek();
  if (value == '\0')
    return value;

  ++pos_;
  if (value == '\r') {
    if (peek() == '\n')
      ++pos_;
    ++line_;
    column_ = 1;
    beginning_of_line_ = true;
  } else if (value == '\n') {
    ++line_;
    column_ = 1;
    beginning_of_line_ = true;
  } else {
    ++column_;
    if (value != ' ' && value != '\t' && value != '\f' && value != '\v')
      beginning_of_line_ = false;
  }
  return value;
}

Location Lexer::location() const { return {file_, line_, column_, pos_}; }

bool Lexer::eof() const { return pos_ >= source_.size(); }

bool Lexer::starts_with(std::string_view text) const {
  return source_.substr(pos_, text.size()) == text;
}

bool Lexer::consume(std::string_view text) {
  if (!starts_with(text))
    return false;
  for (std::size_t index = 0; index < text.size(); ++index)
    take();
  return true;
}

bool Lexer::is_identifier_start(unsigned char value) {
  return value == '_' || std::isalpha(value) || value >= 0x80;
}

bool Lexer::is_identifier_continue(unsigned char value) {
  return is_identifier_start(value) || std::isdigit(value);
}

bool Lexer::is_keyword(std::string_view text) {
  return language_keywords().find(std::string(text)) !=
         language_keywords().end();
}

bool Lexer::is_digit_for_base(char value, int base) {
  switch (base) {
  case 2:
    return is_binary_digit(value);
  case 8:
    return is_octal_digit(value);
  case 10:
    return std::isdigit(static_cast<unsigned char>(value)) != 0;
  case 16:
    return is_hexadecimal_digit(value);
  default:
    return false;
  }
}

bool Lexer::is_integer_suffix(char value) {
  return value == 'u' || value == 'U' || value == 'l' || value == 'L' ||
         value == 'z' || value == 'Z';
}

bool Lexer::is_float_suffix(char value) {
  return value == 'f' || value == 'F' || value == 'l' || value == 'L';
}

bool Lexer::is_punctuation(char value) {
  constexpr std::string_view punctuation = "(){}[];,.:";
  return punctuation.find(value) != std::string_view::npos;
}

void Lexer::skip_horizontal_space() {
  while (peek() == ' ' || peek() == '\t' || peek() == '\f' || peek() == '\v')
    take();
}

void Lexer::skip_whitespace() {
  while (!eof() && std::isspace(static_cast<unsigned char>(peek())))
    take();
}

void Lexer::skip_line_comment() {
  Location start = location();
  if (!consume("//"))
    throw CompileError(start, "internal lexer error: expected line comment");
  while (!eof() && peek() != '\n' && peek() != '\r')
    take();
}

void Lexer::skip_block_comment() {
  Location start = location();
  if (!consume("/*"))
    throw CompileError(start, "internal lexer error: expected block comment");

  unsigned nesting = 1;
  while (!eof() && nesting != 0) {
    if (starts_with("/*")) {
      consume("/*");
      ++nesting;
      continue;
    }
    if (starts_with("*/")) {
      consume("*/");
      --nesting;
      continue;
    }
    take();
  }
  if (nesting != 0)
    throw CompileError(start, "unterminated block comment");
}

Token Lexer::scan_preprocessor() {
  Location start = location();
  std::string spelling;
  bool continued = false;

  do {
    continued = false;
    while (!eof() && peek() != '\n' && peek() != '\r')
      spelling += take();

    std::size_t last = spelling.find_last_not_of(" \t");
    if (last != std::string::npos && spelling[last] == '\\') {
      continued = true;
      if (!eof()) {
        spelling += '\n';
        take();
      }
    }
  } while (continued && !eof());

  return {TokenKind::Preprocessor, std::move(spelling), start};
}

Token Lexer::scan_identifier() {
  Location start = location();
  std::string spelling;

  while (!eof() && is_identifier_continue(static_cast<unsigned char>(peek())))
    spelling += take();

  validate_utf8(spelling, start);
  TokenKind kind = (is_keyword(spelling) || is_cuda_builtin(spelling) ||
                    is_csp_type(spelling))
                       ? TokenKind::Keyword
                       : TokenKind::Identifier;
  return {kind, std::move(spelling), start};
}

std::string Lexer::scan_escape(Location start) {
  std::string result;
  if (take() != '\\')
    throw CompileError(start, "internal lexer error: expected escape");
  result += '\\';

  if (eof())
    throw CompileError(start, "unterminated escape sequence");

  char kind = take();
  result += kind;
  switch (kind) {
  case '\'':
  case '"':
  case '?':
  case '\\':
  case 'a':
  case 'b':
  case 'f':
  case 'n':
  case 'r':
  case 't':
  case 'v':
    return result;
  case 'x': {
    if (!is_hexadecimal_digit(peek()))
      throw CompileError(start, "hex escape requires at least one digit");
    while (is_hexadecimal_digit(peek()))
      result += take();
    return result;
  }
  case 'u':
  case 'U': {
    int required = kind == 'u' ? 4 : 8;
    for (int index = 0; index < required; ++index) {
      if (!is_hexadecimal_digit(peek()))
        throw CompileError(start, "incomplete universal character escape");
      result += take();
    }
    return result;
  }
  case '\n':
  case '\r':
    return result;
  default:
    if (is_octal_digit(kind)) {
      for (int index = 1; index < 3 && is_octal_digit(peek()); ++index)
        result += take();
    }
    return result;
  }
}

Token Lexer::scan_quoted_literal() {
  Location start = location();
  std::string spelling;
  TokenKind kind = TokenKind::String;

  if ((peek() == 'u' && peek(1) == '8' &&
       (peek(2) == '"' || peek(2) == '\''))) {
    spelling += take();
    spelling += take();
  } else if ((peek() == 'u' || peek() == 'U' || peek() == 'L') &&
             (peek(1) == '"' || peek(1) == '\'')) {
    spelling += take();
  }

  char quote = peek();
  if (quote != '"' && quote != '\'')
    throw CompileError(start, "internal lexer error: expected literal");
  if (quote == '\'')
    kind = TokenKind::Character;
  spelling += take();

  bool has_content = false;
  while (!eof()) {
    if (peek() == quote) {
      spelling += take();
      if (kind == TokenKind::Character && !has_content)
        throw CompileError(start, "empty character literal");
      return {kind, std::move(spelling), start};
    }
    if (peek() == '\n' || peek() == '\r')
      throw CompileError(start, "newline in quoted literal");
    if (peek() == '\\')
      spelling += scan_escape(location());
    else
      spelling += take();
    has_content = true;
  }

  throw CompileError(start, "unterminated quoted literal");
}

Token Lexer::scan_raw_string() {
  Location start = location();
  std::string spelling;

  if (peek() == 'u' && peek(1) == '8') {
    spelling += take();
    spelling += take();
  } else if (peek() == 'u' || peek() == 'U' || peek() == 'L') {
    spelling += take();
  }

  if (!consume("R\""))
    throw CompileError(start, "internal lexer error: expected raw string");
  spelling += "R\"";

  std::string delimiter;
  while (!eof() && peek() != '(') {
    char value = peek();
    if (value == ' ' || value == '\t' || value == '\\' || value == ')' ||
        value == '\n' || value == '\r')
      throw CompileError(start, "invalid raw string delimiter");
    if (delimiter.size() == 16)
      throw CompileError(start, "raw string delimiter exceeds 16 characters");
    delimiter += take();
    spelling += value;
  }
  if (!consume("("))
    throw CompileError(start, "unterminated raw string delimiter");
  spelling += '(';

  std::string terminator = ")" + delimiter + "\"";
  while (!eof()) {
    if (starts_with(terminator)) {
      consume(terminator);
      spelling += terminator;
      return {TokenKind::String, std::move(spelling), start};
    }
    spelling += take();
  }
  throw CompileError(start, "unterminated raw string literal");
}

Token Lexer::scan_number() {
  Location start = location();
  std::string spelling;
  int base = 10;
  bool floating = false;

  if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'X')) {
    base = 16;
    spelling += take();
    spelling += take();
  } else if (peek() == '0' && (peek(1) == 'b' || peek(1) == 'B')) {
    base = 2;
    spelling += take();
    spelling += take();
  } else if (peek() == '0' &&
             std::isdigit(static_cast<unsigned char>(peek(1)))) {
    base = 8;
    spelling += take();
  }

  bool saw_digit = false;
  bool previous_separator = false;
  while (is_digit_for_base(peek(), base) || peek() == '\'') {
    if (peek() == '\'') {
      if (!saw_digit || previous_separator)
        throw CompileError(location(), "misplaced digit separator");
      previous_separator = true;
    } else {
      saw_digit = true;
      previous_separator = false;
    }
    spelling += take();
  }

  if (!saw_digit && base != 10)
    throw CompileError(start, "numeric base prefix requires digits");
  if (previous_separator)
    throw CompileError(start, "numeric literal cannot end in a separator");

  if (peek() == '.' && peek(1) != '.') {
    floating = true;
    spelling += take();
    while (is_digit_for_base(peek(), base) || peek() == '\'')
      spelling += take();
  }

  bool exponent = (base == 16 && (peek() == 'p' || peek() == 'P')) ||
                  (base == 10 && (peek() == 'e' || peek() == 'E'));
  if (exponent) {
    floating = true;
    spelling += take();
    if (peek() == '+' || peek() == '-')
      spelling += take();
    if (!std::isdigit(static_cast<unsigned char>(peek())))
      throw CompileError(start, "exponent requires decimal digits");
    while (std::isdigit(static_cast<unsigned char>(peek())) || peek() == '\'')
      spelling += take();
  }

  if (floating) {
    if (is_float_suffix(peek()))
      spelling += take();
  } else {
    while (is_integer_suffix(peek()))
      spelling += take();
  }

  if (is_identifier_start(static_cast<unsigned char>(peek()))) {
    // C++ user-defined numeric literal suffix.
    while (is_identifier_continue(static_cast<unsigned char>(peek())))
      spelling += take();
  }

  return {floating ? TokenKind::Floating : TokenKind::Integer,
          std::move(spelling), start};
}

Token Lexer::scan_operator_or_punctuation() {
  Location start = location();

  for (std::string_view candidate : operators()) {
    if (starts_with(candidate)) {
      consume(candidate);
      TokenKind kind = candidate.size() == 1 && is_punctuation(candidate[0])
                           ? TokenKind::Punctuation
                           : TokenKind::Operator;
      return {kind, std::string(candidate), start};
    }
  }

  constexpr std::string_view single_operators = "+-*/%&|^~!=<>?#";
  if (single_operators.find(peek()) != std::string_view::npos) {
    std::string spelling(1, take());
    return {TokenKind::Operator, std::move(spelling), start};
  }

  unsigned char invalid = static_cast<unsigned char>(peek());
  std::string message = "unexpected character byte 0x";
  constexpr char digits[] = "0123456789ABCDEF";
  message += digits[(invalid >> 4) & 15];
  message += digits[invalid & 15];
  throw CompileError(start, message);
}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> result;
  result.reserve(source_.size() / 4 + 1);

  // Accept a UTF-8 byte-order mark at the beginning of a translation unit.
  // It is metadata rather than part of the first identifier or directive.
  if (pos_ == 0 && source_.size() >= 3 &&
      static_cast<unsigned char>(source_[0]) == 0xEF &&
      static_cast<unsigned char>(source_[1]) == 0xBB &&
      static_cast<unsigned char>(source_[2]) == 0xBF) {
    take();
    take();
    take();
    beginning_of_line_ = true;
  }

  // Tool-friendly C+ files may start with `#!/usr/bin/env cspc`. Native C++
  // does not define shebangs, but accepting one makes executable source files
  // possible without weakening directive handling on later lines.
  if (pos_ <= 3 && starts_with("#!")) {
    while (!eof() && peek() != '\n' && peek() != '\r')
      take();
  }

  while (!eof()) {
    if (std::isspace(static_cast<unsigned char>(peek()))) {
      skip_whitespace();
      continue;
    }
    if (starts_with("//")) {
      skip_line_comment();
      continue;
    }
    if (starts_with("/*")) {
      skip_block_comment();
      continue;
    }
    if (beginning_of_line_ && peek() == '#') {
      result.push_back(scan_preprocessor());
      continue;
    }

    bool raw = starts_with("R\"") || starts_with("u8R\"") ||
               starts_with("uR\"") || starts_with("UR\"") ||
               starts_with("LR\"");
    if (raw) {
      result.push_back(scan_raw_string());
      continue;
    }

    bool prefixed_literal =
        ((peek() == 'u' && peek(1) == '8' &&
          (peek(2) == '"' || peek(2) == '\'')) ||
         ((peek() == 'u' || peek() == 'U' || peek() == 'L') &&
          (peek(1) == '"' || peek(1) == '\'')));
    if (peek() == '"' || peek() == '\'' || prefixed_literal) {
      result.push_back(scan_quoted_literal());
      continue;
    }
    if (is_identifier_start(static_cast<unsigned char>(peek()))) {
      result.push_back(scan_identifier());
      continue;
    }
    if (std::isdigit(static_cast<unsigned char>(peek())) ||
        (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1))))) {
      result.push_back(scan_number());
      continue;
    }
    result.push_back(scan_operator_or_punctuation());
  }

  result.push_back({TokenKind::End, "", location()});
  return result;
}

} // namespace csp
