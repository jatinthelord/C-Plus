#include "tooling.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace csp {
namespace {
std::string json_escape(const std::string &text) {
  std::string result;
  for (unsigned char value : text) {
    if (value == '\\' || value == '"')
      result += '\\';
    if (value == '\n')
      result += "\\n";
    else if (value == '\r')
      result += "\\r";
    else if (value == '\t')
      result += "\\t";
    else
      result += static_cast<char>(value);
  }
  return result;
}

int brace_delta(const std::string &line) {
  int result = 0;
  bool string = false, character = false, escaped = false;
  for (std::size_t index = 0; index < line.size(); ++index) {
    char value = line[index];
    if (!string && !character && value == '/' && index + 1 < line.size() &&
        line[index + 1] == '/')
      break;
    if (escaped) {
      escaped = false;
      continue;
    }
    if ((string || character) && value == '\\') {
      escaped = true;
      continue;
    }
    if (!character && value == '"') {
      string = !string;
      continue;
    }
    if (!string && value == '\'') {
      character = !character;
      continue;
    }
    if (string || character)
      continue;
    if (value == '{')
      ++result;
    else if (value == '}')
      --result;
  }
  return result;
}

std::string node_category(NodeKind kind) {
  if (kind == NodeKind::Class)
    return "class";
  if (kind == NodeKind::Struct)
    return "struct";
  if (kind == NodeKind::Union)
    return "union";
  if (kind == NodeKind::Enum)
    return "enum";
  if (kind == NodeKind::Function)
    return "function";
  return "declaration";
}
} // namespace

std::string format_source(const std::string &source) {
  std::istringstream input(source);
  std::ostringstream output;
  std::string line;
  int indent = 0;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    std::replace(line.begin(), line.end(), '\t', ' ');
    while (!line.empty() &&
           std::isspace(static_cast<unsigned char>(line.back())))
      line.pop_back();
    std::size_t begin = 0;
    while (begin < line.size() &&
           std::isspace(static_cast<unsigned char>(line[begin])))
      ++begin;
    std::string content = line.substr(begin);
    const bool closing = !content.empty() && content.front() == '}';
    if (closing && indent > 0)
      --indent;
    if (!content.empty() && content.front() != '#')
      output << std::string(static_cast<std::size_t>(indent) * 2, ' ');
    output << content << '\n';
    int delta = brace_delta(content);
    if (closing)
      ++delta;
    indent = std::max(0, indent + delta);
  }
  if (source.empty())
    return {};
  return output.str();
}

std::vector<LintIssue> lint_source(const std::string &source,
                                   const std::vector<Token> &tokens,
                                   const std::string &filename) {
  std::vector<LintIssue> issues;
  std::istringstream input(source);
  std::string line;
  std::size_t number = 1;
  while (std::getline(input, line)) {
    if (line.size() > 100)
      issues.push_back({{filename, static_cast<int>(number), 101, 0},
                        "style/line-length",
                        "line exceeds 100 columns"});
    if (line.find('\t') != std::string::npos)
      issues.push_back({{filename, static_cast<int>(number), 1, 0},
                        "style/tabs",
                        "use spaces instead of tabs"});
    if (!line.empty() && (line.back() == ' ' || line.back() == '\t'))
      issues.push_back({{filename, static_cast<int>(number),
                         static_cast<int>(line.size()), 0},
                        "style/trailing-space",
                        "remove trailing whitespace"});
    ++number;
  }
  for (std::size_t index = 0; index < tokens.size(); ++index) {
    const auto &token = tokens[index];
    if (token.text == "goto" || token.text == "Goto")
      issues.push_back(
          {token.location, "safety/goto", "prefer structured control flow"});
    else if (token.text == "new" || token.text == "delete")
      issues.push_back({token.location, "ownership/raw-memory",
                        "prefer cp::owner or a scoped allocator"});
    else if (token.text == "void" && index + 1 < tokens.size() &&
             tokens[index + 1].text == "*")
      issues.push_back(
          {token.location, "safety/void-pointer",
           "prefer a typed pointer or an explicit bugemoan region"});
    else if (token.kind == TokenKind::Preprocessor &&
             token.text.rfind("#define", 0) == 0)
      issues.push_back(
          {token.location, "meta/macro",
           "prefer constexpr, templates, or reflection over a macro"});
  }
  return issues;
}

std::string reflection_json(const Node &program, const std::string &filename) {
  std::ostringstream out;
  out << "{\n  \"source\": \"" << json_escape(filename)
      << "\",\n  \"items\": [";
  bool first = true;
  for (const auto &node : program.children) {
    if (node->kind != NodeKind::Class && node->kind != NodeKind::Struct &&
        node->kind != NodeKind::Union && node->kind != NodeKind::Enum &&
        node->kind != NodeKind::Function)
      continue;
    if (!first)
      out << ',';
    first = false;
    out << "\n    {\"kind\": \"" << node_category(node->kind)
        << "\", \"name\": \"" << json_escape(node->value)
        << "\", \"members\": [";
    bool first_member = true;
    for (const auto &child : node->children) {
      if (child->kind != NodeKind::Declaration &&
          child->kind != NodeKind::Function &&
          child->kind != NodeKind::Parameter)
        continue;
      if (!first_member)
        out << ", ";
      first_member = false;
      out << '"' << json_escape(child->value) << '"';
    }
    out << "]}";
  }
  if (!first)
    out << '\n';
  out << "  ]\n}\n";
  return out.str();
}
} // namespace csp
