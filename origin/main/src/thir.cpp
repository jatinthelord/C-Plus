#include "thir.hpp"
#include <cctype>
#include <ostream>
#include <utility>

namespace csp {
namespace {
std::string thir_trim(std::string text) {
  while (!text.empty() &&
         std::isspace(static_cast<unsigned char>(text.front())))
    text.erase(text.begin());
  while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
    text.pop_back();
  return text;
}
void print_node(const THIRNode &node, std::ostream &out, int depth) {
  out << std::string(static_cast<std::size_t>(depth) * 2, ' ') << '%' << node.id
      << " = " << node_name(node.source_kind) << " : "
      << (node.type.name.empty() ? thir_type_name(node.type.kind)
                                 : node.type.name);
  if (node.type.constant)
    out << " const";
  if (node.type.reference)
    out << " ref";
  if (!node.text.empty())
    out << " \"" << node.text << '"';
  out << " at " << node.location.line << ':' << node.location.column << '\n';
  for (const auto &child : node.children)
    print_node(child, out, depth + 1);
}
} // namespace
const char *thir_type_name(THIRTypeKind kind) {
  switch (kind) {
  case THIRTypeKind::Unknown:
    return "unknown";
  case THIRTypeKind::Void:
    return "void";
  case THIRTypeKind::Boolean:
    return "bool";
  case THIRTypeKind::SignedInteger:
    return "int";
  case THIRTypeKind::UnsignedInteger:
    return "unsigned";
  case THIRTypeKind::FloatingPoint:
    return "double";
  case THIRTypeKind::Character:
    return "char";
  case THIRTypeKind::String:
    return "const char*";
  case THIRTypeKind::Pointer:
    return "pointer";
  case THIRTypeKind::Named:
    return "named";
  }
  return "unknown";
}
std::string THIRBuilder::declaration_name(const std::string &text) {
  std::size_t end = text.find_first_of("=([{;|");
  if (end == std::string::npos)
    end = text.size();
  while (end && std::isspace(static_cast<unsigned char>(text[end - 1])))
    --end;
  std::size_t begin = end;
  while (begin && (std::isalnum(static_cast<unsigned char>(text[begin - 1])) ||
                   text[begin - 1] == '_'))
    --begin;
  return text.substr(begin, end - begin);
}
std::string THIRBuilder::declaration_type(const std::string &text) {
  const auto name = declaration_name(text);
  const auto position = text.find(name);
  return position == std::string::npos ? std::string{}
                                       : thir_trim(text.substr(0, position));
}
THIRType THIRBuilder::named_type(std::string spelling) {
  spelling = thir_trim(std::move(spelling));
  THIRType result;
  result.name = spelling;
  result.constant = spelling.rfind("const ", 0) == 0;
  if (spelling.find('*') != std::string::npos)
    result.kind = THIRTypeKind::Pointer;
  else if (spelling == "void")
    result.kind = THIRTypeKind::Void;
  else if (spelling == "bool")
    result.kind = THIRTypeKind::Boolean;
  else if (spelling == "float" || spelling == "double" ||
           spelling == "long double")
    result.kind = THIRTypeKind::FloatingPoint;
  else if (spelling == "char" || spelling == "char8_t" ||
           spelling == "char16_t" || spelling == "char32_t")
    result.kind = THIRTypeKind::Character;
  else if (spelling.find("unsigned") != std::string::npos || spelling == "u8" ||
           spelling == "u16" || spelling == "u32" || spelling == "u64")
    result.kind = THIRTypeKind::UnsignedInteger;
  else if (spelling == "int" || spelling == "short" || spelling == "long" ||
           spelling == "long long")
    result.kind = THIRTypeKind::SignedInteger;
  else if (!spelling.empty() && spelling != "auto" && spelling != "let" &&
           spelling != "let mut")
    result.kind = THIRTypeKind::Named;
  return result;
}
THIRType THIRBuilder::lookup(const std::string &name) const {
  for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
    auto found = scope->find(name);
    if (found != scope->end())
      return found->second;
  }
  return {};
}
THIRType THIRBuilder::infer(const Node &node,
                            const std::vector<THIRNode> &children) {
  if (node.kind == NodeKind::Literal) {
    THIRType type;
    type.constant = true;
    if (node.value == "true" || node.value == "false")
      type.kind = THIRTypeKind::Boolean;
    else if (!node.value.empty() && node.value.front() == '\"')
      type.kind = THIRTypeKind::String;
    else if (!node.value.empty() && node.value.front() == '\'')
      type.kind = THIRTypeKind::Character;
    else if (node.value.find_first_of(".eEpP") != std::string::npos)
      type.kind = THIRTypeKind::FloatingPoint;
    else
      type.kind = THIRTypeKind::SignedInteger;
    type.name = thir_type_name(type.kind);
    return type;
  }
  if (node.kind == NodeKind::Identifier)
    return lookup(node.value);
  if (node.kind == NodeKind::NullptrExpression)
    return {THIRTypeKind::Pointer, "nullptr_t", true, false};
  if (node.kind == NodeKind::Declaration || node.kind == NodeKind::Parameter) {
    auto type = named_type(declaration_type(node.value));
    if (type.kind == THIRTypeKind::Unknown) {
      if (!children.empty())
        type = children.front().type;
      else if (const auto equals = node.value.find('=');
               equals != std::string::npos) {
          const std::string value = thir_trim(node.value.substr(equals + 1));
        if (value == "true" || value == "false")
          type = named_type("bool");
        else if (!value.empty() && value.front() == '\"')
          type = {THIRTypeKind::String, "const char*", true, false};
        else if (value.find_first_of(".eEpP") != std::string::npos)
          type = named_type("double");
        else {
          type = lookup(value);
          if (type.kind == THIRTypeKind::Unknown)
            type = named_type("int");
        }
      }
      if (type.kind != THIRTypeKind::Unknown)
        ++inferred_nodes_;
    }
    return type;
  }
  if (node.kind == NodeKind::UnaryExpression && !children.empty()) {
    auto type = children.front().type;
    if (node.value == "!")
      type = named_type("bool");
    else if (node.value == "&") {
      type.kind = THIRTypeKind::Pointer;
      type.name += "*";
    }
    return type;
  }
  if (node.kind == NodeKind::BinaryExpression && children.size() == 2) {
    if (node.value == "==" || node.value == "!=" || node.value == "<" ||
        node.value == "<=" || node.value == ">" || node.value == ">=" ||
        node.value == "&&" || node.value == "||")
      return named_type("bool");
    if (children[0].type.kind == THIRTypeKind::FloatingPoint ||
        children[1].type.kind == THIRTypeKind::FloatingPoint)
      return named_type("double");
    return children[0].type;
  }
  if ((node.kind == NodeKind::AssignmentExpression ||
       node.kind == NodeKind::ConditionalExpression) &&
      !children.empty())
    return children.back().type;
  if (node.kind == NodeKind::Return && !children.empty())
    return children.front().type;
  return {};
}
THIRNode THIRBuilder::lower(const Node &node) {
  const bool scoped = node.kind == NodeKind::Function ||
                      node.kind == NodeKind::Block ||
                      node.kind == NodeKind::BugemoanBlock;
  if (scoped)
    scopes_.emplace_back();
  THIRNode result;
  result.id = next_id_++;
  result.source_kind = node.kind;
  result.location = node.location;
  result.text = node.value;
  for (const auto &child : node.children)
    result.children.push_back(lower(*child));
  result.type = infer(node, result.children);
  if ((node.kind == NodeKind::Declaration ||
       node.kind == NodeKind::Parameter) &&
      !scopes_.empty()) {
    const auto name = declaration_name(node.value);
    if (!name.empty())
      scopes_.back()[name] = result.type;
  }
  if (scoped)
    scopes_.pop_back();
  return result;
}
THIRModule THIRBuilder::build(const Node &program, std::string source_name) {
  next_id_ = 0;
  inferred_nodes_ = 0;
  scopes_.clear();
  THIRModule result;
  result.source_name = std::move(source_name);
  scopes_.emplace_back();
  for (const auto &child : program.children)
    result.declarations.push_back(lower(*child));
  scopes_.pop_back();
  result.inferred_nodes = inferred_nodes_;
  return result;
}
void THIRModule::print(std::ostream &out) const {
  out << "thir.module \"" << source_name << "\" inferred " << inferred_nodes
      << '\n';
  for (const auto &node : declarations)
    print_node(node, out, 1);
}
} // namespace csp
