#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace csp::cpasm {

struct SourceLocation {
  std::string file;
  std::size_t line = 1;
  std::size_t column = 1;
};

enum class TokenKind {
  Identifier,
  Number,
  String,
  Directive,
  Comma,
  Colon,
  LeftBracket,
  RightBracket,
  Plus,
  Minus,
  Hash,
  Newline,
  End
};

struct Token {
  TokenKind kind = TokenKind::End;
  std::string text;
  SourceLocation location;
};

class Error : public std::runtime_error {
public:
  SourceLocation location;
  Error(SourceLocation location, std::string message);
};

class Lexer {
  std::string_view source_;
  std::string file_;
  std::size_t offset_ = 0;
  std::size_t line_ = 1;
  std::size_t column_ = 1;

  char peek(std::size_t lookahead = 0) const;
  char take();
  SourceLocation location() const;
  Token lex_word();
  Token lex_number();
  Token lex_string();

public:
  Lexer(std::string_view source, std::string file = "<cpasm>");
  std::vector<Token> tokenize();
};

enum class Architecture { X86_64, AArch64 };
enum class StatementKind { Directive, Label, Instruction };

struct Operand {
  std::vector<Token> tokens;
  std::string text() const;
};

struct Statement {
  StatementKind kind = StatementKind::Instruction;
  SourceLocation location;
  std::string name;
  std::vector<Operand> operands;
};

struct Program {
  Architecture architecture = Architecture::X86_64;
  std::vector<Statement> statements;
};

class Parser {
  std::vector<Token> tokens_;
  std::size_t current_ = 0;

  const Token &peek(std::size_t lookahead = 0) const;
  Token take();
  bool match(TokenKind kind);
  void finish_line();
  std::vector<Operand> parse_operands();
  Statement parse_statement();

public:
  explicit Parser(std::vector<Token> tokens);
  Program parse();
};

class Emitter {
public:
  static std::string emit_gnu(const Program &program);
};

const char *architecture_name(Architecture architecture);

} // namespace csp::cpasm
