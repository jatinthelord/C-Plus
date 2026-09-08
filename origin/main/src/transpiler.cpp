#include "transpiler.hpp"
#include "diagnostic.hpp"
#include "lowering.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace csp {
std::string Transpiler::indent(unsigned depth) {
  return std::string(depth * 4, ' ');
}

std::string Transpiler::rust_type(const std::string &type) {
  if (type == "void")
    return "()";
  if (type == "bool")
    return "bool";
  if (type == "char")
    return "i8";
  if (type == "short")
    return "i16";
  if (type == "int" || type == "signed")
    return "i32";
  if (type == "unsigned")
    return "u32";
  if (type == "long")
    return "i64";
  if (type == "float")
    return "f32";
  if (type == "double")
    return "f64";
  if (type == "u8")
    return "u8";
  if (type == "u16")
    return "u16";
  if (type == "u32")
    return "u32";
  if (type == "u64")
    return "u64";
  if (type == "u128")
    return "u128";
  if (type == "size_t")
    return "usize";
  return type;
}

std::string Transpiler::rust_literal(const std::string &literal,
                                     bool c_string) {
  if (!c_string || literal.empty() || literal.front() != '"')
    return literal;
  std::string contents = literal.substr(1, literal.size() - 2);
  return "b\"" + contents + "\\0\".as_ptr() as *const c_char";
}

std::string Transpiler::rust_expression(const Node &node) {
  switch (node.kind) {
  case NodeKind::Identifier:
  case NodeKind::Literal:
  case NodeKind::ThisExpression:
  case NodeKind::NullptrExpression:
    return node.value == "nullptr" ? "core::ptr::null_mut()" : node.value;
  case NodeKind::UnaryExpression:
    return node.value + rust_expression(*node.children.at(0));
  case NodeKind::PostfixExpression:
    if (node.value == "++")
      return "{ " + rust_expression(*node.children.at(0)) + " += 1; " +
             rust_expression(*node.children.at(0)) + " }";
    if (node.value == "--")
      return "{ " + rust_expression(*node.children.at(0)) + " -= 1; " +
             rust_expression(*node.children.at(0)) + " }";
    return rust_expression(*node.children.at(0));
  case NodeKind::BinaryExpression:
  case NodeKind::AssignmentExpression:
    return "(" + rust_expression(*node.children.at(0)) + " " + node.value +
           " " + rust_expression(*node.children.at(1)) + ")";
  case NodeKind::ConditionalExpression:
    return "(if " + rust_expression(*node.children.at(0)) + " { " +
           rust_expression(*node.children.at(1)) + " } else { " +
           rust_expression(*node.children.at(2)) + " })";
  case NodeKind::CallExpression: {
    std::string callee = rust_expression(*node.children.at(0));
    bool print = callee == "printc";
    std::string result = print ? "unsafe { printf(" : callee + "(";
    for (std::size_t index = 1; index < node.children.size(); ++index) {
      if (index != 1)
        result += ", ";
      const Node &argument = *node.children[index];
      if (print && index == 1 && argument.kind == NodeKind::Literal)
        result += rust_literal(argument.value, true);
      else
        result += rust_expression(argument);
    }
    result += print ? ") }" : ")";
    return result;
  }
  case NodeKind::IndexExpression:
    return rust_expression(*node.children.at(0)) + "[" +
           rust_expression(*node.children.at(1)) + " as usize]";
  case NodeKind::MemberExpression:
    return rust_expression(*node.children.at(0)) + node.value;
  case NodeKind::InitializerList: {
    std::string result = "[";
    for (std::size_t index = 0; index < node.children.size(); ++index) {
      if (index)
        result += ", ";
      result += rust_expression(*node.children[index]);
    }
    return result + "]";
  }
  default:
    throw CompileError(node.location,
                       std::string("Rust emitter does not support ") +
                           node_name(node.kind) + " expression yet");
  }
}

void Transpiler::rust_statement(const Node &node, std::string &output,
                                unsigned depth) {
  const std::string margin = indent(depth);
  switch (node.kind) {
  case NodeKind::Block:
    output += "{\n";
    for (const auto &child : node.children)
      rust_statement(*child, output, depth + 1);
    output += margin + "}";
    return;
  case NodeKind::Declaration: {
    std::istringstream stream(node.value);
    std::string type, name, equals;
    stream >> type >> name >> equals;
    if (type == "const") {
      stream >> type >> name >> equals;
    }
    if (name.empty())
      throw CompileError(node.location,
                         "Rust emitter cannot determine declaration name");
    output += margin + "let mut " + name + ": " + rust_type(type);
    std::string initializer;
    std::getline(stream, initializer);
    if (equals == "=") {
      while (!initializer.empty() &&
             std::isspace(static_cast<unsigned char>(initializer.front())))
        initializer.erase(initializer.begin());
      output += " = " + initializer;
    } else {
      output += " = Default::default()";
    }
    output += ";\n";
    return;
  }
  case NodeKind::ExpressionStatement:
    output += margin + rust_expression(*node.children.at(0)) + ";\n";
    return;
  case NodeKind::Return:
  case NodeKind::CoroutineReturn:
    output += margin + "return";
    if (!node.children.empty())
      output += " " + rust_expression(*node.children[0]);
    output += ";\n";
    return;
  case NodeKind::If:
    output += margin + "if " + rust_expression(*node.children.at(0)) + " ";
    rust_statement(*node.children.at(1), output, depth);
    if (node.children.size() > 2) {
      output += " else ";
      rust_statement(*node.children.at(2), output, depth);
    }
    output += "\n";
    return;
  case NodeKind::While:
    output += margin + "while " + rust_expression(*node.children.at(0)) + " ";
    rust_statement(*node.children.at(1), output, depth);
    output += "\n";
    return;
  case NodeKind::For: {
    // C+ shorthand value: `int i < limit , i ++`.
    std::istringstream stream(node.value);
    std::string type, name, less;
    stream >> type >> name >> less;
    std::string limit;
    std::getline(stream, limit, ',');
    while (!limit.empty() &&
           std::isspace(static_cast<unsigned char>(limit.front())))
      limit.erase(limit.begin());
    while (!limit.empty() &&
           std::isspace(static_cast<unsigned char>(limit.back())))
      limit.pop_back();
    if (type != "int" || less != "<")
      throw CompileError(node.location,
                         "Rust emitter currently requires C+ short for syntax");
    output += margin + "for " + name + " in 0.." + limit + " ";
    rust_statement(*node.children.back(), output, depth);
    output += "\n";
    return;
  }
  case NodeKind::Break:
    output += margin + "break;\n";
    return;
  case NodeKind::Continue:
    output += margin + "continue;\n";
    return;
  case NodeKind::EmptyStatement:
    return;
  default:
    throw CompileError(node.location,
                       std::string("Rust emitter does not support ") +
                           node_name(node.kind) + " statement yet");
  }
}

void Transpiler::rust_function(const Node &node, std::string &output) {
  std::istringstream signature(node.value);
  std::string return_type, name;
  signature >> return_type >> name;
  if (name.empty())
    throw CompileError(node.location,
                       "Rust emitter cannot determine function name");
  bool main = name == "main";
  output += "fn " + std::string(main ? "csp_main" : name) + "(";
  bool first = true;
  const Node *body = nullptr;
  for (const auto &child : node.children) {
    if (child->kind == NodeKind::Parameter) {
      std::istringstream parameter(child->value);
      std::string type, parameter_name;
      parameter >> type >> parameter_name;
      if (!first)
        output += ", ";
      output += parameter_name + ": " + rust_type(type);
      first = false;
    } else if (child->kind == NodeKind::Block) {
      body = child.get();
    }
  }
  output += ") -> " + rust_type(return_type) + " ";
  if (!body)
    output += ";\n";
  else {
    rust_statement(*body, output, 0);
    output += "\n\n";
  }
}

std::string Transpiler::emit_rust(const Node &program) {
  std::string output = "// Generated by the C+ Rust transpiler.\n"
                       "use std::ffi::{c_char, c_int};\n"
                       "unsafe extern \"C\" { fn printf(format: *const c_char, "
                       "...) -> c_int; }\n\n";
  bool has_main = false;
  for (const auto &child : program.children) {
    if (child->kind == NodeKind::Preprocessor)
      continue;
    if (child->kind != NodeKind::Function)
      throw CompileError(
          child->location,
          "Rust emitter currently supports functions at top level");
    std::istringstream signature(child->value);
    std::string return_type, name;
    signature >> return_type >> name;
    has_main |= name == "main";
    rust_function(*child, output);
  }
  if (has_main)
    output += "fn main() { std::process::exit(csp_main()); }\n";
  return output;
}

std::string Transpiler::emit_c(const std::string &lowered,
                               const std::vector<Token> &tokens) {
  for (const Token &token : tokens) {
    static const std::vector<std::string> unsupported = {
        "class", "namespace", "template", "new",        "delete", "try",
        "catch", "throw",     "::",       "__global__", "<<<",    ">>>"};
    if (std::find(unsupported.begin(), unsupported.end(), token.text) !=
        unsupported.end())
      throw CompileError(token.location, "construct '" + token.text +
                                             "' cannot be represented in C");
  }
  std::string source = lowered;
  std::vector<SourceEdit> edits;
  for (const Token &token : tokens) {
    if (token.text == "nullptr")
      edits.push_back({token.location.offset,
                       token.location.offset + token.text.size(), "NULL",
                       "lower nullptr to C"});
  }
  std::sort(edits.begin(), edits.end(),
            [](const SourceEdit &a, const SourceEdit &b) {
              return a.begin > b.begin;
            });
  for (const auto &edit : edits)
    source.replace(edit.begin, edit.end - edit.begin, edit.replacement);
  return "/* Generated by the C+ C transpiler. */\n"
         "#include <stdbool.h>\n#include <stddef.h>\n#include "
         "<stdint.h>\n#include <stdio.h>\n"
         "typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;\n"
         "typedef uint64_t u64; typedef __uint128_t u128; typedef uint32_t "
         "unichar;\n"
         "#define printc printf\n" +
         source + "\n";
}
} // namespace csp
