#include "parser.hpp"
#include "diagnostic.hpp"
#include <algorithm>

namespace csp {
const Token &Parser::peek(std::size_t n) const {
  return t_[std::min(p_ + n, t_.size() - 1)];
}
bool Parser::accept(const std::string &s) {
  if (peek().text == s) {
    ++p_;
    return true;
  }
  return false;
}
const Token &Parser::require(const std::string &s) {
  if (peek().text != s)
    throw CompileError(peek().location,
                       "expected '" + s + "', found '" + peek().text + "'");
  return t_[p_++];
}
std::string Parser::collect_until(const std::string &end, bool consume) {
  std::string s;
  int par = 0, br = 0;
  while (peek().kind != TokenKind::End) {
    if (par == 0 && br == 0 && peek().text == end)
      break;
    if (peek().text == "(")
      ++par;
    if (peek().text == ")")
      --par;
    if (peek().text == "[")
      ++br;
    if (peek().text == "]")
      --br;
    if (!s.empty())
      s += ' ';
    s += t_[p_++].text;
  }
  if (consume)
    require(end);
  return s;
}

int Parser::precedence(const std::string &op) {
  if (is_assignment_operator(op))
    return 1;
  if (op == "||")
    return 3;
  if (op == "&&")
    return 4;
  if (op == "|")
    return 5;
  if (op == "^")
    return 6;
  if (op == "&")
    return 7;
  if (op == "==" || op == "!=")
    return 8;
  if (op == "<" || op == ">" || op == "<=" || op == ">=")
    return 9;
  if (op == "<<" || op == ">>")
    return 10;
  if (op == "+" || op == "-")
    return 11;
  if (op == "*" || op == "/" || op == "%")
    return 12;
  return 0;
}
bool Parser::right_associative(const std::string &op) {
  return precedence(op) == 1;
}

std::unique_ptr<Node> Parser::prefix() {
  Token token = peek();
  if (accept("this"))
    return postfix(
        std::make_unique<Node>(NodeKind::ThisExpression, token.location));
  if (accept("nullptr"))
    return postfix(
        std::make_unique<Node>(NodeKind::NullptrExpression, token.location));
  if (peek().text == "sizeof" || peek().text == "alignof" ||
      peek().text == "typeid") {
    ++p_;
    NodeKind kind = NodeKind::SizeofExpression;
    if (token.text == "alignof")
      kind = NodeKind::AlignofExpression;
    else if (token.text == "typeid")
      kind = NodeKind::TypeidExpression;
    auto node = std::make_unique<Node>(kind, token.location);
    if (accept("(")) {
      node->children.push_back(expression());
      require(")");
    } else {
      node->children.push_back(prefix());
    }
    return postfix(std::move(node));
  }
  if (accept("new")) {
    auto node = std::make_unique<Node>(NodeKind::NewExpression, token.location);
    std::string type_name;
    while (!at_end() && peek().text != "(" && peek().text != "[" &&
           peek().text != ";" && peek().text != "," && peek().text != ")") {
      if (!type_name.empty())
        type_name += ' ';
      type_name += peek().text;
      ++p_;
    }
    node->value = type_name;
    return postfix(std::move(node));
  }
  if (accept("delete")) {
    auto node =
        std::make_unique<Node>(NodeKind::DeleteExpression, token.location);
    if (accept("[")) {
      require("]");
      node->value = "array";
    }
    node->children.push_back(prefix());
    return node;
  }
  if (accept("[")) {
    auto node =
        std::make_unique<Node>(NodeKind::LambdaExpression, token.location);
    std::string captures;
    int nested_brackets = 0;
    while (!at_end()) {
      if (peek().text == "]" && nested_brackets == 0)
        break;
      if (peek().text == "[")
        ++nested_brackets;
      else if (peek().text == "]")
        --nested_brackets;
      if (!captures.empty())
        captures += ' ';
      captures += peek().text;
      ++p_;
    }
    require("]");
    node->value = captures;

    if (peek().text == "(") {
      auto parameters = parse_parameter_list();
      for (auto &parameter : parameters)
        node->children.push_back(std::move(parameter));
    }

    std::string specifiers;
    while (!at_end() && peek().text != "{") {
      if (!specifiers.empty())
        specifiers += ' ';
      specifiers += peek().text;
      ++p_;
    }
    if (!specifiers.empty())
      node->value += " | " + specifiers;
    node->children.push_back(block());
    return postfix(std::move(node));
  }
  if (token.text == "+" || token.text == "-" || token.text == "!" ||
      token.text == "~" || token.text == "*" || token.text == "&" ||
      token.text == "++" || token.text == "--") {
    ++p_;
    auto n = std::make_unique<Node>(NodeKind::UnaryExpression, token.location,
                                    token.text);
    n->children.push_back(prefix());
    return n;
  }
  if (accept("(")) {
    auto n = expression();
    require(")");
    return postfix(std::move(n));
  }
  if (accept("{")) {
    auto n = std::make_unique<Node>(NodeKind::InitializerList, token.location);
    if (!accept("}")) {
      do {
        n->children.push_back(expression());
      } while (accept(","));
      require("}");
    }
    return n;
  }
  if (token.kind == TokenKind::Identifier || token.kind == TokenKind::Keyword) {
    ++p_;
    return postfix(std::make_unique<Node>(NodeKind::Identifier, token.location,
                                          token.text));
  }
  if (token.kind == TokenKind::Integer || token.kind == TokenKind::Floating ||
      token.kind == TokenKind::String || token.kind == TokenKind::Character) {
    ++p_;
    return postfix(
        std::make_unique<Node>(NodeKind::Literal, token.location, token.text));
  }
  throw CompileError(token.location,
                     "expected expression, found '" + token.text + "'");
}

std::unique_ptr<Node> Parser::postfix(std::unique_ptr<Node> left) {
  for (;;) {
    Location at = peek().location;
    if (accept("(")) {
      auto call = std::make_unique<Node>(NodeKind::CallExpression, at);
      call->children.push_back(std::move(left));
      if (!accept(")")) {
        for (;;) {
          call->children.push_back(expression());
          if (accept(")"))
            break;
          // C+ print formatting allows adjacent arguments: printc("%d" value).
          accept(",");
        }
      }
      left = std::move(call);
    } else if (accept("<<<")) {
      auto launch =
          std::make_unique<Node>(NodeKind::KernelLaunchExpression, at);
      launch->children.push_back(std::move(left));
      if (!accept(">>>")) {
        do {
          launch->children.push_back(expression());
        } while (accept(","));
        require(">>>");
      }
      require("(");
      if (!accept(")")) {
        do {
          launch->children.push_back(expression());
        } while (accept(","));
        require(")");
      }
      left = std::move(launch);
    } else if (peek().text == "<" && left->kind == NodeKind::Identifier) {
      const std::size_t saved = p_;
      std::string arguments;
      int depth = 0;
      do {
        const std::string text = peek().text;
        if (text == "<") ++depth;
        else if (text == ">") --depth;
        if (!arguments.empty() && text != ">" && arguments.back() != '<')
          arguments += ' ';
        arguments += text;
        ++p_;
      } while (!at_end() && depth > 0);
      if (depth != 0 || peek().text != "(") {
        p_ = saved;
        break;
      }
      left->value += arguments;
    } else if (accept("[")) {
      auto index = std::make_unique<Node>(NodeKind::IndexExpression, at);
      index->children.push_back(std::move(left));
      index->children.push_back(expression());
      require("]");
      left = std::move(index);
    } else if (peek().text == "." || peek().text == "->" ||
               peek().text == "::") {
      std::string op = peek().text;
      ++p_;
      Token member = peek();
      if (member.kind != TokenKind::Identifier && member.kind != TokenKind::Keyword)
        throw CompileError(member.location, "expected member name");
      ++p_;
      auto n = std::make_unique<Node>(NodeKind::MemberExpression, at,
                                      op + member.text);
      n->children.push_back(std::move(left));
      left = std::move(n);
    } else if (peek().text == "++" || peek().text == "--") {
      std::string op = peek().text;
      ++p_;
      auto n = std::make_unique<Node>(NodeKind::PostfixExpression, at, op);
      n->children.push_back(std::move(left));
      left = std::move(n);
    } else
      break;
  }
  return left;
}

std::unique_ptr<Node> Parser::expression(int minp) {
  auto left = prefix();
  while (true) {
    std::string op = peek().text;
    int prec = precedence(op);
    if (prec < minp)
      break;
    Token oper = peek();
    ++p_;
    int next = prec + (right_associative(op) ? 0 : 1);
    auto right = expression(next);
    NodeKind kind =
        prec == 1 ? NodeKind::AssignmentExpression : NodeKind::BinaryExpression;
    auto n = std::make_unique<Node>(kind, oper.location, op);
    n->children.push_back(std::move(left));
    n->children.push_back(std::move(right));
    left = std::move(n);
  }
  if (minp == 1 && accept("?")) {
    auto n = std::make_unique<Node>(NodeKind::ConditionalExpression,
                                    peek().location);
    n->children.push_back(std::move(left));
    n->children.push_back(expression());
    require(":");
    n->children.push_back(expression());
    return n;
  }
  return left;
}

std::unique_ptr<Node> Parser::expression_until(const std::string &delimiter) {
  auto n = expression();
  require(delimiter);
  return n;
}
std::unique_ptr<Node> Parser::block() {
  auto at = require("{").location;
  auto n = std::make_unique<Node>(NodeKind::Block, at);
  while (peek().text != "}") {
    if (peek().kind == TokenKind::End)
      throw CompileError(peek().location, "unterminated block");
    if (!diagnostics_) {
      n->children.push_back(statement());
      continue;
    }
    try {
      n->children.push_back(statement());
    } catch (const CompileError &error) {
      diagnostics_->error(error.location, error.what());
      if (diagnostics_->reached_error_limit())
        throw;
      std::size_t before = p_;
      synchronize_statement();
      if (p_ == before && peek().text != "}" && !at_end())
        ++p_;
    }
  }
  require("}");
  return n;
}

bool Parser::at_end() const { return peek().kind == TokenKind::End; }

bool Parser::token_starts_type(const Token &token) const {
  static const std::vector<std::string> types = {
      "auto",     "bool",     "char",     "char8_t",  "char16_t",
      "char32_t", "const",    "decltype", "double",   "float",
      "int",      "long",     "short",    "signed",   "size_t",
      "u8",       "u16",      "u32",      "u64",      "u128",
      "unichar",  "unsigned", "void",     "volatile", "wchar_t",
      "let"};
  return std::find(types.begin(), types.end(), token.text) != types.end();
}

bool Parser::token_starts_expression(const Token &token) const {
  if (token.kind == TokenKind::Identifier || token.kind == TokenKind::Integer ||
      token.kind == TokenKind::Floating || token.kind == TokenKind::String ||
      token.kind == TokenKind::Character)
    return true;
  static const std::vector<std::string> starts = {
      "(",      "{",    "+",    "-",     "!",       "~",      "*",
      "&",      "++",   "--",   "new",   "delete",  "sizeof", "alignof",
      "typeid", "this", "true", "false", "nullptr", "["};
  return std::find(starts.begin(), starts.end(), token.text) != starts.end();
}

std::string Parser::collect_balanced(const std::string &open,
                                     const std::string &close) {
  require(open);
  int depth = 1;
  std::string result;
  while (!at_end() && depth > 0) {
    if (peek().text == open) {
      ++depth;
    } else if (peek().text == close) {
      --depth;
      if (depth == 0) {
        ++p_;
        break;
      }
    }
    if (!result.empty())
      result += ' ';
    result += t_[p_++].text;
  }
  if (depth != 0)
    throw CompileError(peek().location, "unterminated '" + open + "' group");
  return result;
}

void Parser::synchronize_statement() {
  int brace_depth = 0;
  while (!at_end()) {
    if (peek().text == "{") {
      ++brace_depth;
    } else if (peek().text == "}") {
      if (brace_depth == 0)
        return;
      --brace_depth;
    } else if (peek().text == ";" && brace_depth == 0) {
      ++p_;
      return;
    }
    ++p_;
  }
}

std::unique_ptr<Node> Parser::parse_if() {
  Location at = require("if").location;
  auto node = std::make_unique<Node>(NodeKind::If, at);
  require("(");
  node->children.push_back(expression_until(")"));
  node->children.push_back(statement());
  if (accept("else"))
    node->children.push_back(statement());
  return node;
}

std::unique_ptr<Node> Parser::parse_while() {
  Location at = require("while").location;
  auto node = std::make_unique<Node>(NodeKind::While, at);
  require("(");
  node->children.push_back(expression_until(")"));
  node->children.push_back(statement());
  return node;
}

std::unique_ptr<Node> Parser::parse_do_while() {
  Location at = require("do").location;
  auto node = std::make_unique<Node>(NodeKind::DoWhile, at);
  node->children.push_back(statement());
  require("while");
  require("(");
  node->children.push_back(expression_until(")"));
  require(";");
  return node;
}

std::unique_ptr<Node> Parser::parse_for() {
  Location at = require("for").location;
  auto node = std::make_unique<Node>(NodeKind::For, at);
  require("(");

  // The header remains structurally separated into init, condition, and step.
  // This accepts both ISO C++ and C+'s `int i < n, i++;` shorthand.
  std::string first = collect_until(";", false);
  if (peek().text == ";") {
    ++p_;
    if (accept(")")) {
      node->value = first;
      node->children.push_back(statement());
      return node;
    }
    node->children.push_back(
        std::make_unique<Node>(NodeKind::Declaration, at, first));
    if (!accept(";"))
      node->children.push_back(expression_until(";"));
    else
      node->children.push_back(
          std::make_unique<Node>(NodeKind::EmptyStatement, at));
    if (!accept(")"))
      node->children.push_back(expression_until(")"));
  } else {
    // collect_until may reach ')' for a range-for or C+ shorthand.
    std::string remainder = collect_until(")");
    node->value = first + remainder;
  }
  node->children.push_back(statement());
  return node;
}

std::unique_ptr<Node> Parser::parse_switch() {
  Location at = require("switch").location;
  auto node = std::make_unique<Node>(NodeKind::Switch, at);
  require("(");
  node->children.push_back(expression_until(")"));
  node->children.push_back(statement());
  return node;
}

std::unique_ptr<Node> Parser::parse_match() {
  Location at = require("match").location;
  auto node = std::make_unique<Node>(NodeKind::Match, at);
  require("(");
  node->children.push_back(expression_until(")"));
  node->children.push_back(statement());
  return node;
}

std::unique_ptr<Node> Parser::parse_case() {
  Location at = peek().location;
  if (accept("default")) {
    require(":");
    return std::make_unique<Node>(NodeKind::Default, at);
  }
  require("case");
  auto node = std::make_unique<Node>(NodeKind::Case, at);
  node->children.push_back(expression_until(":"));
  return node;
}

std::unique_ptr<Node> Parser::parse_try() {
  Location at = require("try").location;
  auto node = std::make_unique<Node>(NodeKind::Try, at);
  node->children.push_back(block());
  if (peek().text != "catch")
    throw CompileError(peek().location,
                       "try statement requires a catch handler");
  while (accept("catch")) {
    Location catch_at = t_[p_ - 1].location;
    auto handler = std::make_unique<Node>(NodeKind::Catch, catch_at);
    handler->value = collect_balanced("(", ")");
    handler->children.push_back(block());
    node->children.push_back(std::move(handler));
  }
  return node;
}

std::unique_ptr<Node> Parser::parse_throw() {
  Location at = require("throw").location;
  auto node = std::make_unique<Node>(NodeKind::Throw, at);
  if (!accept(";"))
    node->children.push_back(expression_until(";"));
  return node;
}

std::unique_ptr<Node> Parser::parse_return(NodeKind kind) {
  Location at = peek().location;
  ++p_;
  auto node = std::make_unique<Node>(kind, at);
  if (!accept(";"))
    node->children.push_back(expression_until(";"));
  return node;
}

std::unique_ptr<Node> Parser::parse_static_assert() {
  Location at = require("static_assert").location;
  auto node = std::make_unique<Node>(NodeKind::StaticAssert, at);
  require("(");
  node->children.push_back(expression());
  if (accept(",")) {
    if (peek().kind != TokenKind::String)
      throw CompileError(peek().location,
                         "static_assert message must be a string literal");
    node->value = peek().text;
    ++p_;
  }
  require(")");
  require(";");
  return node;
}

std::unique_ptr<Node> Parser::parse_asm() {
  Location at = require("asm").location;
  auto node = std::make_unique<Node>(NodeKind::AsmStatement, at);
  node->value = collect_balanced("(", ")");
  require(";");
  return node;
}
std::unique_ptr<Node> Parser::statement() {
  Location at = peek().location;
  if (accept("bugemoan")) {
    auto guarded = block();
    guarded->kind = NodeKind::BugemoanBlock;
    return guarded;
  }
  if (peek().text == "{")
    return block();
  if (accept(";"))
    return std::make_unique<Node>(NodeKind::EmptyStatement, at);
  if (peek().text == "if")
    return parse_if();
  if (peek().text == "while")
    return parse_while();
  if (peek().text == "do")
    return parse_do_while();
  if (peek().text == "for")
    return parse_for();
  if (peek().text == "switch")
    return parse_switch();
  if (peek().text == "match")
    return parse_match();
  if (peek().text == "case" || peek().text == "default")
    return parse_case();
  if (peek().text == "try")
    return parse_try();
  if (peek().text == "throw")
    return parse_throw();
  if (peek().text == "return")
    return parse_return();
  if (peek().text == "co_return")
    return parse_return(NodeKind::CoroutineReturn);
  if (peek().text == "co_yield")
    return parse_return(NodeKind::CoroutineYield);
  if (peek().text == "static_assert")
    return parse_static_assert();
  if (peek().text == "asm")
    return parse_asm();
  if (accept("break")) {
    require(";");
    return std::make_unique<Node>(NodeKind::Break, at);
  }
  if (accept("continue")) {
    require(";");
    return std::make_unique<Node>(NodeKind::Continue, at);
  }
  if (accept("Goto") || accept("goto")) {
    auto n = std::make_unique<Node>(NodeKind::Goto, at, collect_until(";"));
    return n;
  }
  if (peek().kind == TokenKind::Identifier && peek(1).text == ":") {
    std::string name = peek().text;
    p_ += 2;
    return std::make_unique<Node>(NodeKind::Label, at, name);
  }
  if (token_starts_type(peek()))
    return std::make_unique<Node>(NodeKind::Declaration, at,
                                  collect_until(";"));
  auto n = std::make_unique<Node>(NodeKind::ExpressionStatement, at);
  n->children.push_back(expression_until(";"));
  return n;
}

std::unique_ptr<Node> Parser::parse_namespace() {
  Location at = require("namespace").location;
  auto node = std::make_unique<Node>(NodeKind::Namespace, at);

  if (accept("{")) {
    node->value = "<anonymous>";
  } else {
    if (peek().kind != TokenKind::Identifier)
      throw CompileError(peek().location, "expected namespace name or '{'");
    node->value = peek().text;
    ++p_;
    while (accept("::")) {
      if (peek().kind != TokenKind::Identifier)
        throw CompileError(peek().location,
                           "expected identifier after namespace '::'");
      node->value += "::" + peek().text;
      ++p_;
    }
    require("{");
  }

  while (!accept("}")) {
    if (at_end())
      throw CompileError(at, "unterminated namespace definition");
    node->children.push_back(top_level());
  }
  return node;
}

std::unique_ptr<Node> Parser::parse_record(NodeKind kind) {
  Location at = peek().location;
  ++p_;
  auto node = std::make_unique<Node>(kind, at);

  if (peek().kind == TokenKind::Identifier) {
    node->value = peek().text;
    ++p_;
  }

  // Preserve base clauses with their access and virtual specifiers. A later
  // semantic pass resolves each named base in the enclosing scope.
  if (accept(":")) {
    auto bases = std::make_unique<Node>(NodeKind::Declaration, peek().location,
                                        collect_until("{", false));
    node->children.push_back(std::move(bases));
  }

  if (accept(";"))
    return node;
  require("{");
  while (!accept("}")) {
    if (at_end())
      throw CompileError(at, "unterminated record definition");
    if ((peek().text == "public" || peek().text == "protected" ||
         peek().text == "private") &&
        peek(1).text == ":") {
      auto access =
          std::make_unique<Node>(NodeKind::Label, peek().location, peek().text);
      p_ += 2;
      node->children.push_back(std::move(access));
      continue;
    }
    node->children.push_back(top_level());
  }
  accept(";");
  return node;
}

std::unique_ptr<Node> Parser::parse_enum() {
  Location at = require("enum").location;
  auto node = std::make_unique<Node>(NodeKind::Enum, at);

  if (accept("class"))
    node->value = "class ";
  else if (accept("struct"))
    node->value = "struct ";

  if (peek().kind == TokenKind::Identifier) {
    node->value += peek().text;
    ++p_;
  }

  if (accept(":")) {
    auto underlying = std::make_unique<Node>(
        NodeKind::Declaration, peek().location, collect_until("{", false));
    node->children.push_back(std::move(underlying));
  }
  if (accept(";"))
    return node;

  require("{");
  while (!accept("}")) {
    if (peek().kind != TokenKind::Identifier)
      throw CompileError(peek().location, "expected enumerator name");
    Location item_at = peek().location;
    std::string name = peek().text;
    ++p_;
    auto item = std::make_unique<Node>(NodeKind::Declaration, item_at, name);
    if (accept("="))
      item->children.push_back(expression());
    node->children.push_back(std::move(item));
    if (!accept(",")) {
      require("}");
      break;
    }
    if (accept("}"))
      break;
  }
  accept(";");
  return node;
}

std::unique_ptr<Node> Parser::parse_using() {
  Location at = require("using").location;
  bool namespace_directive = accept("namespace");
  auto node = std::make_unique<Node>(
      namespace_directive ? NodeKind::Using : NodeKind::TypeAlias, at);
  node->value = collect_until(";");
  return node;
}

std::unique_ptr<Node> Parser::parse_template() {
  Location at = require("template").location;
  auto node = std::make_unique<Node>(NodeKind::Template, at);
  node->value = collect_balanced("<", ">");
  node->children.push_back(top_level());
  return node;
}

std::vector<std::unique_ptr<Node>> Parser::parse_parameter_list() {
  std::vector<std::unique_ptr<Node>> parameters;
  require("(");
  if (accept(")"))
    return parameters;

  while (!at_end()) {
    Location at = peek().location;
    std::string spelling;
    int parentheses = 0;
    int brackets = 0;
    int angles = 0;
    while (!at_end()) {
      const std::string &text = peek().text;
      if (parentheses == 0 && brackets == 0 && angles == 0 &&
          (text == "," || text == ")"))
        break;
      if (text == "(")
        ++parentheses;
      else if (text == ")")
        --parentheses;
      else if (text == "[")
        ++brackets;
      else if (text == "]")
        --brackets;
      else if (text == "<")
        ++angles;
      else if (text == ">" && angles > 0)
        --angles;
      if (!spelling.empty())
        spelling += ' ';
      spelling += text;
      ++p_;
    }
    if (spelling.empty())
      throw CompileError(peek().location, "expected function parameter");
    parameters.push_back(
        std::make_unique<Node>(NodeKind::Parameter, at, spelling));
    if (accept(")"))
      return parameters;
    require(",");
    if (accept(")"))
      throw CompileError(peek().location,
                         "trailing comma is not valid in a parameter list");
  }
  throw CompileError(peek().location, "unterminated function parameter list");
}

std::unique_ptr<Node> Parser::parse_external_declaration() {
  Location at = peek().location;
  std::string prefix;
  int angles = 0;

  while (!at_end()) {
    const std::string &text = peek().text;
    if (angles == 0 && (text == "(" || text == ";" || text == "="))
      break;
    if (text == "<")
      ++angles;
    else if (text == ">" && angles > 0)
      --angles;
    if (!prefix.empty())
      prefix += ' ';
    prefix += text;
    ++p_;
  }

  if (accept(";"))
    return std::make_unique<Node>(NodeKind::Declaration, at, prefix);

  if (accept("=")) {
    auto declaration =
        std::make_unique<Node>(NodeKind::Declaration, at, prefix);
    declaration->children.push_back(expression_until(";"));
    return declaration;
  }

  if (peek().text != "(")
    throw CompileError(
        peek().location,
        "expected function parameters or declaration terminator");

  auto function = std::make_unique<Node>(NodeKind::Function, at, prefix);
  auto parameters = parse_parameter_list();
  for (auto &parameter : parameters)
    function->children.push_back(std::move(parameter));

  // Accept cv/ref qualifiers, noexcept, trailing returns, requires clauses,
  // CUDA launch bounds, and override/final specifiers before the body.
  std::string suffix;
  while (!at_end() && peek().text != "{" && peek().text != ";" &&
         peek().text != "=") {
    if (!suffix.empty())
      suffix += ' ';
    suffix += peek().text;
    ++p_;
  }
  if (!suffix.empty())
    function->value += " | " + suffix;

  if (accept("=")) {
    function->value += " = " + collect_until(";");
    return function;
  }
  if (accept(";"))
    return function;
  function->children.push_back(block());
  return function;
}

std::unique_ptr<Node> Parser::top_level() {
  Location at = peek().location;
  if (peek().kind == TokenKind::Preprocessor) {
    std::string s = peek().text;
    ++p_;
    return std::make_unique<Node>(NodeKind::Preprocessor, at, s);
  }
  if (peek().text == "namespace")
    return parse_namespace();
  if (peek().text == "class")
    return parse_record(NodeKind::Class);
  if (peek().text == "struct")
    return parse_record(NodeKind::Struct);
  if (peek().text == "union")
    return parse_record(NodeKind::Union);
  if (peek().text == "enum")
    return parse_enum();
  if (peek().text == "using")
    return parse_using();
  if (peek().text == "template")
    return parse_template();
  if (peek().text == "static_assert")
    return parse_static_assert();
  return parse_external_declaration();
}
std::unique_ptr<Node> Parser::parse() {
  diagnostics_ = nullptr;
  auto root = std::make_unique<Node>(NodeKind::Program, peek().location);
  while (peek().kind != TokenKind::End)
    root->children.push_back(top_level());
  return root;
}

std::unique_ptr<Node> Parser::parse_recovering(DiagnosticEngine &diagnostics) {
  diagnostics_ = &diagnostics;
  auto root = std::make_unique<Node>(NodeKind::Program, peek().location);
  while (!at_end()) {
    try {
      root->children.push_back(top_level());
    } catch (const CompileError &error) {
      diagnostics.error(error.location, error.what());
      if (diagnostics.reached_error_limit())
        break;
      std::size_t before = p_;
      synchronize_statement();
      if (p_ == before && !at_end())
        ++p_;
    }
  }
  diagnostics_ = nullptr;
  return root;
}
} // namespace csp
