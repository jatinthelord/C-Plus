#pragma once
#include <cstddef>
#include <string>

namespace csp {
struct Location {
  std::string file;
  int line = 1, column = 1;
  std::size_t offset = 0;
};
enum class TokenKind {
  Identifier,
  Keyword,
  Integer,
  Floating,
  String,
  Character,
  Preprocessor,
  Punctuation,
  Operator,
  End
};
struct Token {
  TokenKind kind;
  std::string text;
  Location location;
};
inline const char *token_name(TokenKind k) {
  switch (k) {
  case TokenKind::Identifier:
    return "identifier";
  case TokenKind::Keyword:
    return "keyword";
  case TokenKind::Integer:
    return "integer";
  case TokenKind::Floating:
    return "floating";
  case TokenKind::String:
    return "string";
  case TokenKind::Character:
    return "character";
  case TokenKind::Preprocessor:
    return "preprocessor";
  case TokenKind::Punctuation:
    return "punctuation";
  case TokenKind::Operator:
    return "operator";
  case TokenKind::End:
    return "end";
  }
  return "unknown";
}
} // namespace csp
