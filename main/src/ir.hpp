#pragma once

#include "thir.hpp"
#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

namespace csp {

enum class HIROp {
  Module,
  Function,
  Parameter,
  Block,
  Declaration,
  Expression,
  If,
  Loop,
  Switch,
  Case,
  Return,
  Break,
  Continue,
  Goto,
  Label,
  Throw,
  Try,
  Assembly,
  Namespace,
  Record,
  Enumeration,
  Directive,
  Unknown
};

struct HIRNode {
  std::size_t id = 0;
  HIROp operation = HIROp::Unknown;
  Location location;
  std::string text;
  std::vector<HIRNode> children;
};

struct HIRModule {
  std::string source_name;
  std::vector<HIRNode> declarations;
  std::size_t node_count = 0;
  void print(std::ostream &output) const;
};

class HIRBuilder {
  std::size_t next_id_ = 0;
  HIRNode lower(const THIRNode &node);
  static HIROp operation_for(NodeKind kind);

public:
  HIRModule build(const THIRModule &program);
};

enum class MIROp {
  Nop,
  Declare,
  Evaluate,
  Branch,
  BranchIf,
  Switch,
  Jump,
  Return,
  Throw,
  Call,
  InlineAssembly,
  Unreachable
};

struct MIRInstruction {
  MIROp operation = MIROp::Nop;
  std::string result;
  std::vector<std::string> operands;
  Location location;
};

struct MIRBlock {
  std::size_t id = 0;
  std::string name;
  std::vector<MIRInstruction> instructions;
  bool terminated = false;
};

struct MIRFunction {
  std::string signature;
  std::vector<MIRBlock> blocks;
};

struct MIRModule {
  std::string source_name;
  std::vector<std::string> globals;
  std::vector<MIRFunction> functions;
  void print(std::ostream &output) const;
  bool verify(std::string &error) const;
};

class MIRBuilder {
  MIRFunction *function_ = nullptr;
  MIRBlock *block_ = nullptr;
  std::size_t next_block_ = 0;
  std::size_t next_temporary_ = 0;
  MIRBlock &make_block(std::string name);
  void select(std::size_t block);
  void emit(MIROp operation, const HIRNode &source,
            std::vector<std::string> operands = {}, std::string result = {});
  void lower_node(const HIRNode &node);
  void lower_if(const HIRNode &node);
  void lower_loop(const HIRNode &node);
  void lower_switch(const HIRNode &node);
  void lower_function(const HIRNode &node, MIRModule &module);

public:
  MIRModule build(const HIRModule &hir);
};

const char *hir_name(HIROp operation);
const char *mir_name(MIROp operation);

} // namespace csp
