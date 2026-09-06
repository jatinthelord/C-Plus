#include "lowering.hpp"
#include "diagnostic.hpp"
#include <algorithm>
#include <cctype>

namespace csp {
void Lowering::add_edit(std::size_t begin, std::size_t end,
                        std::string replacement, std::string reason) {
  edits_.push_back({begin, end, std::move(replacement), std::move(reason)});
}

std::size_t Lowering::matching(std::size_t start, const std::string &open,
                               const std::string &close) const {
  if (start >= tokens_.size() || tokens_[start].text != open)
    return tokens_.size();
  int depth = 0;
  for (std::size_t index = start; index < tokens_.size(); ++index) {
    if (tokens_[index].text == open)
      ++depth;
    else if (tokens_[index].text == close && --depth == 0)
      return index;
  }
  return tokens_.size();
}

std::string Lowering::source_between(std::size_t first,
                                     std::size_t last) const {
  if (first >= tokens_.size() || last >= tokens_.size() || first > last)
    return {};
  std::size_t begin = tokens_[first].location.offset;
  std::size_t end = tokens_[last].location.offset + tokens_[last].text.size();
  return source_.substr(begin, end - begin);
}

bool Lowering::starts_expression(const Token &token) {
  if (token.kind == TokenKind::Identifier || token.kind == TokenKind::Integer ||
      token.kind == TokenKind::Floating || token.kind == TokenKind::String ||
      token.kind == TokenKind::Character)
    return true;
  return token.text == "(" || token.text == "{" || token.text == "[" ||
         token.text == "+" || token.text == "-" || token.text == "!" ||
         token.text == "~" || token.text == "*" || token.text == "&" ||
         token.text == "++" || token.text == "--" || token.text == "new" ||
         token.text == "sizeof" || token.text == "this" ||
         token.text == "nullptr" || token.text == "true" ||
         token.text == "false";
}

void Lowering::lower_runtime_include() {
  for (const Token &token : tokens_) {
    if (token.kind != TokenKind::Preprocessor)
      continue;
    std::string canonical;
    for (unsigned char character : token.text)
      if (!std::isspace(character))
        canonical += static_cast<char>(character);
    if (canonical == "#include<cpstream>")
      add_edit(token.location.offset, token.location.offset + token.text.size(),
               "", "cpstream is supplied by the C+ runtime");
  }
}

void Lowering::lower_goto_spelling() {
  for (std::size_t index = 0; index < tokens_.size(); ++index) {
    const Token &token = tokens_[index];
    if (token.text == "__hopa__")
      add_edit(token.location.offset, token.location.offset + token.text.size(),
               "", "lower hopa bootstrap marker");
    else if (token.text == "match")
      add_edit(token.location.offset, token.location.offset + token.text.size(),
               "switch", "lower exhaustive match to native switch");
    else if (token.text == "bugemoan" && index + 1 < tokens_.size() &&
             tokens_[index + 1].text == "{")
      add_edit(token.location.offset, token.location.offset + token.text.size(),
               "", "lower bugemoan capability block");
    else if (token.text == "Goto")
      add_edit(token.location.offset, token.location.offset + token.text.size(),
               "goto", "normalize C+ Goto spelling");
  }
}

void Lowering::lower_print_arguments() {
  for (std::size_t index = 0; index + 3 < tokens_.size(); ++index) {
    if (tokens_[index].text != "printc" || tokens_[index + 1].text != "(")
      continue;
    std::size_t close = matching(index + 1, "(", ")");
    if (close == tokens_.size())
      continue;
    int nested = 0;
    for (std::size_t current = index + 2; current + 1 < close; ++current) {
      const Token &left = tokens_[current];
      const Token &right = tokens_[current + 1];
      if (left.text == "(" || left.text == "[" || left.text == "{")
        ++nested;
      if (left.text == ")" || left.text == "]" || left.text == "}")
        --nested;
      if (nested == 0 && left.kind == TokenKind::String &&
          starts_expression(right) && right.text != ",") {
        std::size_t left_end = left.location.offset + left.text.size();
        std::string trivia =
            source_.substr(left_end, right.location.offset - left_end);
        bool comments = trivia.find("//") != std::string::npos ||
                        trivia.find("/*") != std::string::npos;
        if (comments)
          add_edit(left_end, left_end, ",",
                   "insert C+ implicit print separator");
        else
          add_edit(left_end, right.location.offset, ", ",
                   "insert C+ implicit print argument separator");
      }
    }
    index = close;
  }
}

void Lowering::lower_short_for_loops() {
  for (std::size_t index = 0; index + 7 < tokens_.size(); ++index) {
    if (tokens_[index].text != "for" || tokens_[index + 1].text != "(")
      continue;
    std::size_t close = matching(index + 1, "(", ")");
    if (close == tokens_.size())
      continue;

    std::size_t comma = tokens_.size();
    std::size_t semicolon = tokens_.size();
    int nested = 0;
    for (std::size_t current = index + 2; current < close; ++current) {
      const std::string &text = tokens_[current].text;
      if (text == "(" || text == "[" || text == "{")
        ++nested;
      else if (text == ")" || text == "]" || text == "}")
        --nested;
      else if (nested == 0 && text == "," && comma == tokens_.size())
        comma = current;
      else if (nested == 0 && text == ";")
        semicolon = current;
    }
    if (comma == tokens_.size() || semicolon + 1 != close)
      continue;
    if (tokens_[index + 2].text != "int" || index + 5 > comma)
      continue;
    const Token &name = tokens_[index + 3];
    if (name.kind != TokenKind::Identifier || tokens_[index + 4].text != "<")
      continue;

    std::string limit = source_between(index + 5, comma - 1);
    std::string increment = source_between(comma + 1, semicolon - 1);
    std::string replacement = "for(int " + name.text + " = 0; " + name.text +
                              " < " + limit + "; " + increment + ")";
    std::size_t begin = tokens_[index].location.offset;
    std::size_t end =
        tokens_[close].location.offset + tokens_[close].text.size();
    add_edit(begin, end, std::move(replacement), "lower C+ short for loop");
    index = close;
  }
}

void Lowering::lower_array_initializers() {
  for (std::size_t index = 0; index + 7 < tokens_.size(); ++index) {
    if (tokens_[index].kind != TokenKind::Keyword &&
        tokens_[index].kind != TokenKind::Identifier)
      continue;
    if (tokens_[index + 1].kind != TokenKind::Identifier ||
        tokens_[index + 2].text != "[")
      continue;
    std::size_t bracket = matching(index + 2, "[", "]");
    if (bracket == tokens_.size() || bracket + 2 >= tokens_.size() ||
        tokens_[bracket + 1].text != "=")
      continue;
    if (tokens_[bracket + 2].text == "{")
      continue;

    std::size_t semicolon = bracket + 2;
    bool comma = false;
    int nested = 0;
    for (; semicolon < tokens_.size(); ++semicolon) {
      const std::string &text = tokens_[semicolon].text;
      if (text == "(" || text == "[" || text == "{")
        ++nested;
      else if (text == ")" || text == "]" || text == "}")
        --nested;
      else if (text == "," && nested == 0)
        comma = true;
      else if (text == ";" && nested == 0)
        break;
    }
    if (!comma || semicolon >= tokens_.size())
      continue;
    std::size_t after_equal = tokens_[bracket + 1].location.offset + 1;
    add_edit(after_equal, after_equal, " {", "open C+ array initializer");
    add_edit(tokens_[semicolon].location.offset,
             tokens_[semicolon].location.offset, "}",
             "close C+ array initializer");
    index = semicolon;
  }
}

void Lowering::lower_bindings() {
  for (std::size_t index = 0; index < tokens_.size(); ++index) {
    if (tokens_[index].text != "let")
      continue;
    if (index + 1 < tokens_.size() && tokens_[index + 1].text == "mut") {
      add_edit(tokens_[index].location.offset,
               tokens_[index + 1].location.offset +
                   tokens_[index + 1].text.size(),
               "auto", "lower mutable inferred binding");
      ++index;
    } else {
      add_edit(tokens_[index].location.offset,
               tokens_[index].location.offset + tokens_[index].text.size(),
               "const auto", "lower immutable inferred binding");
    }
  }
}

void Lowering::lower_optional_types() {
  for (std::size_t index = 0; index + 2 < tokens_.size(); ++index) {
    const Token &type = tokens_[index];
    if (tokens_[index + 1].text != "?" ||
        tokens_[index + 2].kind != TokenKind::Identifier)
      continue;
    const bool declaration_position =
        index == 0 || tokens_[index - 1].text == ";" ||
        tokens_[index - 1].text == "{" || tokens_[index - 1].text == "(" ||
        tokens_[index - 1].text == ",";
    if (!declaration_position ||
        (type.kind != TokenKind::Identifier && type.kind != TokenKind::Keyword))
      continue;
    add_edit(
        type.location.offset,
        tokens_[index + 1].location.offset + tokens_[index + 1].text.size(),
        "std::optional<" + type.text + ">", "lower explicit optional type");
    index += 2;
  }
}

std::string Lowering::run() {
  edits_.clear();
  lower_runtime_include();
  lower_goto_spelling();
  lower_print_arguments();
  lower_short_for_loops();
  lower_array_initializers();
  lower_bindings();
  lower_optional_types();

  std::sort(edits_.begin(), edits_.end(),
            [](const SourceEdit &left, const SourceEdit &right) {
              if (left.begin != right.begin)
                return left.begin > right.begin;
              return left.end > right.end;
            });

  std::string result = source_;
  std::size_t previous_begin = source_.size() + 1;
  for (const SourceEdit &edit : edits_) {
    if (edit.end > source_.size() || edit.begin > edit.end)
      throw CompileError({"<lowering>", 1, 1, edit.begin},
                         "invalid source edit range");
    if (edit.end > previous_begin)
      throw CompileError({"<lowering>", 1, 1, edit.begin},
                         "overlapping source edits");
    result.replace(edit.begin, edit.end - edit.begin, edit.replacement);
    previous_begin = edit.begin;
  }
  return result;
}
} // namespace csp
