#pragma once
#include "ast.hpp"
#include <cstddef>
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

namespace csp {
enum class THIRTypeKind {
  Unknown,
  Void,
  Boolean,
  SignedInteger,
  UnsignedInteger,
  FloatingPoint,
  Character,
  String,
  Pointer,
  Named
};
struct THIRType {
  THIRTypeKind kind = THIRTypeKind::Unknown;
  std::string name;
  bool constant = false;
  bool reference = false;
};
struct THIRNode {
  std::size_t id = 0;
  NodeKind source_kind = NodeKind::Program;
  Location location;
  std::string text;
  THIRType type;
  std::vector<THIRNode> children;
};
struct THIRModule {
  std::string source_name;
  std::vector<THIRNode> declarations;
  std::size_t inferred_nodes = 0;
  void print(std::ostream &output) const;
};
class THIRBuilder {
  std::size_t next_id_ = 0, inferred_nodes_ = 0;
  std::vector<std::unordered_map<std::string, THIRType>> scopes_;
  THIRNode lower(const Node &node);
  THIRType infer(const Node &node, const std::vector<THIRNode> &children);
  THIRType lookup(const std::string &name) const;
  static THIRType named_type(std::string spelling);
  static std::string declaration_name(const std::string &text);
  static std::string declaration_type(const std::string &text);

public:
  THIRModule build(const Node &program, std::string source_name);
};
const char *thir_type_name(THIRTypeKind kind);
} // namespace csp
