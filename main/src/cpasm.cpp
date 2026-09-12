#include "cpasm.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace csp::cpasm {
namespace {
bool word_start(char value) {
  const auto c = static_cast<unsigned char>(value);
  return std::isalpha(c) || value == '_' || value == '%' || value == '$';
}
bool word_continue(char value) {
  const auto c = static_cast<unsigned char>(value);
  return std::isalnum(c) || value == '_' || value == '.' || value == '%' ||
         value == '$' || value == '@';
}
bool number_continue(char value) {
  const auto c = static_cast<unsigned char>(value);
  return std::isalnum(c) || value == 'x' || value == 'X' || value == '_';
}
std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  });
  return value;
}
std::string join_operands(const std::vector<Operand> &operands) {
  std::string result;
  for (std::size_t index = 0; index < operands.size(); ++index) {
    if (index)
      result += ", ";
    result += operands[index].text();
  }
  return result;
}
} // namespace

Error::Error(SourceLocation source, std::string message)
    : std::runtime_error(source.file + ':' + std::to_string(source.line) + ':' +
                         std::to_string(source.column) + ": " + message),
      location(std::move(source)) {}

Lexer::Lexer(std::string_view source, std::string file)
    : source_(source), file_(std::move(file)) {}

char Lexer::peek(std::size_t lookahead) const {
  return offset_ + lookahead < source_.size() ? source_[offset_ + lookahead]
                                               : '\0';
}
char Lexer::take() {
  const char result = peek();
  if (result == '\0')
    return result;
  ++offset_;
  if (result == '\n') {
    ++line_;
    column_ = 1;
  } else {
    ++column_;
  }
  return result;
}
SourceLocation Lexer::location() const { return {file_, line_, column_}; }

Token Lexer::lex_word() {
  const auto start = location();
  std::string value;
  while (word_continue(peek()))
    value += take();
  return {TokenKind::Identifier, std::move(value), start};
}
Token Lexer::lex_number() {
  const auto start = location();
  std::string value;
  while (number_continue(peek()))
    value += take();
  return {TokenKind::Number, std::move(value), start};
}
Token Lexer::lex_string() {
  const auto start = location();
  std::string value;
  value += take();
  bool escaped = false;
  while (peek() != '\0') {
    const char current = take();
    value += current;
    if (current == '\n' && !escaped)
      throw Error(start, "unterminated string literal");
    if (current == '"' && !escaped)
      return {TokenKind::String, std::move(value), start};
    if (current == '\\' && !escaped)
      escaped = true;
    else
      escaped = false;
  }
  throw Error(start, "unterminated string literal");
}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> result;
  while (peek() != '\0') {
    if (peek() == ' ' || peek() == '\t' || peek() == '\r') {
      take();
      continue;
    }
    if (peek() == ';' || (peek() == '/' && peek(1) == '/')) {
      while (peek() != '\0' && peek() != '\n')
        take();
      continue;
    }
    const auto start = location();
    if (peek() == '\n') {
      take();
      result.push_back({TokenKind::Newline, "\n", start});
    } else if (peek() == '"') {
      result.push_back(lex_string());
    } else if (peek() == '.' && word_start(peek(1))) {
      std::string value;
      value += take();
      while (word_continue(peek()))
        value += take();
      result.push_back({TokenKind::Directive, std::move(value), start});
    } else if (word_start(peek())) {
      result.push_back(lex_word());
    } else if (std::isdigit(static_cast<unsigned char>(peek()))) {
      result.push_back(lex_number());
    } else {
      const char value = take();
      TokenKind kind;
      switch (value) {
      case ',': kind = TokenKind::Comma; break;
      case ':': kind = TokenKind::Colon; break;
      case '[': kind = TokenKind::LeftBracket; break;
      case ']': kind = TokenKind::RightBracket; break;
      case '+': kind = TokenKind::Plus; break;
      case '-': kind = TokenKind::Minus; break;
      case '#': kind = TokenKind::Hash; break;
      default: throw Error(start, std::string("unexpected character '") + value + "'");
      }
      result.push_back({kind, std::string(1, value), start});
    }
  }
  result.push_back({TokenKind::End, "", location()});
  return result;
}

std::string Operand::text() const {
  std::string result;
  for (const auto &token : tokens) {
    const bool punctuation = token.kind == TokenKind::RightBracket ||
                             token.kind == TokenKind::Plus ||
                             token.kind == TokenKind::Minus ||
                             token.kind == TokenKind::Hash;
    if (!result.empty() && !punctuation && result.back() != '[' &&
        result.back() != '+' && result.back() != '-' && result.back() != '#')
      result += ' ';
    result += token.text;
  }
  return result;
}

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}
const Token &Parser::peek(std::size_t lookahead) const {
  const auto index = std::min(current_ + lookahead, tokens_.size() - 1);
  return tokens_[index];
}
Token Parser::take() { return tokens_[current_++]; }
bool Parser::match(TokenKind kind) {
  if (peek().kind != kind)
    return false;
  ++current_;
  return true;
}
void Parser::finish_line() {
  if (peek().kind != TokenKind::Newline && peek().kind != TokenKind::End)
    throw Error(peek().location, "unexpected token at end of statement");
  match(TokenKind::Newline);
}

std::vector<Operand> Parser::parse_operands() {
  std::vector<Operand> result;
  if (peek().kind == TokenKind::Newline || peek().kind == TokenKind::End)
    return result;
  int brackets = 0;
  result.push_back({});
  while (peek().kind != TokenKind::Newline && peek().kind != TokenKind::End) {
    if (peek().kind == TokenKind::Comma && brackets == 0) {
      take();
      if (result.back().tokens.empty())
        throw Error(peek().location, "empty operand");
      result.push_back({});
      continue;
    }
    Token token = take();
    if (token.kind == TokenKind::LeftBracket)
      ++brackets;
    else if (token.kind == TokenKind::RightBracket && --brackets < 0)
      throw Error(token.location, "unmatched closing bracket");
    result.back().tokens.push_back(std::move(token));
  }
  if (brackets != 0)
    throw Error(result.back().tokens.front().location, "unclosed memory operand");
  if (result.back().tokens.empty())
    throw Error(peek().location, "empty operand");
  return result;
}

Statement Parser::parse_statement() {
  const Token first = take();
  if (first.kind != TokenKind::Identifier && first.kind != TokenKind::Directive)
    throw Error(first.location, "expected instruction, label, or directive");
  if (first.kind == TokenKind::Identifier && match(TokenKind::Colon)) {
    finish_line();
    return {StatementKind::Label, first.location, first.text, {}};
  }
  const auto kind = first.kind == TokenKind::Directive
                        ? StatementKind::Directive
                        : StatementKind::Instruction;
  auto operands = parse_operands();
  finish_line();
  return {kind, first.location, lower(first.text), std::move(operands)};
}

Program Parser::parse() {
  Program program;
  while (peek().kind != TokenKind::End) {
    if (match(TokenKind::Newline))
      continue;
    Statement statement = parse_statement();
    if (statement.kind == StatementKind::Directive && statement.name == ".target") {
      if (statement.operands.size() != 1)
        throw Error(statement.location, ".target expects one architecture");
      const auto name = lower(statement.operands[0].text());
      if (name == "x86_64" || name == "x64" || name == "amd64")
        program.architecture = Architecture::X86_64;
      else if (name == "aarch64" || name == "arm64")
        program.architecture = Architecture::AArch64;
      else
        throw Error(statement.location, "unsupported CP ASM target '" + name + "'");
    }
    program.statements.push_back(std::move(statement));
  }
  return program;
}

std::string Emitter::emit_gnu(const Program &program) {
  static const std::unordered_set<std::string> directives = {
      ".target", ".section", ".global", ".extern", ".align", ".byte",
      ".word", ".dword", ".qword", ".ascii", ".asciz", ".const",
      ".proc", ".end"};
  std::ostringstream output;
  output << "# Generated from CP ASM (.cpsm)\n";
  if (program.architecture == Architecture::X86_64)
    output << ".intel_syntax noprefix\n";
  for (const auto &statement : program.statements) {
    if (statement.kind == StatementKind::Label) {
      output << statement.name << ":\n";
      continue;
    }
    if (statement.kind == StatementKind::Instruction) {
      output << "  " << statement.name;
      if (!statement.operands.empty())
        output << ' ' << join_operands(statement.operands);
      output << '\n';
      continue;
    }
    if (!directives.contains(statement.name))
      throw Error(statement.location, "unknown CP ASM directive '" + statement.name + "'");
    const auto args = join_operands(statement.operands);
    if (statement.name == ".target" || statement.name == ".end")
      continue;
    if (statement.name == ".proc") {
      if (statement.operands.size() != 1)
        throw Error(statement.location, ".proc expects one symbol name");
      output << ".text\n.globl " << args << '\n' << args << ":\n";
    } else if (statement.name == ".global")
      output << ".globl " << args << '\n';
    else if (statement.name == ".const")
      output << ".equ " << args << '\n';
    else if (statement.name == ".dword")
      output << ".long " << args << '\n';
    else if (statement.name == ".qword")
      output << ".quad " << args << '\n';
    else if (statement.name == ".section" && args == "text")
      output << ".text\n";
    else if (statement.name == ".section" && args == "data")
      output << ".data\n";
    else if (statement.name == ".section" && args == "rodata")
      output << ".section .rodata\n";
    else
      output << statement.name << (args.empty() ? "" : " " + args) << '\n';
  }
  return output.str();
}

const char *architecture_name(Architecture architecture) {
  return architecture == Architecture::X86_64 ? "x86_64" : "aarch64";
}

} // namespace csp::cpasm
