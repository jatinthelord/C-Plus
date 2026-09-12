#include "ir.hpp"

#include <ostream>
#include <sstream>
#include <utility>

namespace csp {
namespace {
void print_hir_node(const HIRNode &node, std::ostream &output, int depth) {
  output << std::string(static_cast<std::size_t>(depth) * 2, ' ') << '%'
         << node.id << " = " << hir_name(node.operation);
  if (!node.text.empty())
    output << " \"" << node.text << '"';
  output << " at " << node.location.line << ':' << node.location.column << '\n';
  for (const auto &child : node.children)
    print_hir_node(child, output, depth + 1);
}

std::string operand_text(const HIRNode &node) {
  if (!node.text.empty())
    return node.text;
  if (!node.children.empty())
    return operand_text(node.children.front());
  return hir_name(node.operation);
}
} // namespace

const char *hir_name(HIROp operation) {
  switch (operation) {
  case HIROp::Module:
    return "module";
  case HIROp::Function:
    return "function";
  case HIROp::Parameter:
    return "parameter";
  case HIROp::Block:
    return "block";
  case HIROp::Declaration:
    return "declare";
  case HIROp::Expression:
    return "expression";
  case HIROp::If:
    return "if";
  case HIROp::Loop:
    return "loop";
  case HIROp::Switch:
    return "switch";
  case HIROp::Case:
    return "case";
  case HIROp::Return:
    return "return";
  case HIROp::Break:
    return "break";
  case HIROp::Continue:
    return "continue";
  case HIROp::Goto:
    return "goto";
  case HIROp::Label:
    return "label";
  case HIROp::Throw:
    return "throw";
  case HIROp::Try:
    return "try";
  case HIROp::Assembly:
    return "assembly";
  case HIROp::Namespace:
    return "namespace";
  case HIROp::Record:
    return "record";
  case HIROp::Enumeration:
    return "enum";
  case HIROp::Directive:
    return "directive";
  case HIROp::Unknown:
    return "unknown";
  }
  return "unknown";
}

const char *mir_name(MIROp operation) {
  switch (operation) {
  case MIROp::Nop:
    return "nop";
  case MIROp::Declare:
    return "declare";
  case MIROp::Evaluate:
    return "eval";
  case MIROp::Branch:
    return "branch";
  case MIROp::BranchIf:
    return "branch_if";
  case MIROp::Switch:
    return "switch";
  case MIROp::Jump:
    return "jump";
  case MIROp::Return:
    return "return";
  case MIROp::Throw:
    return "throw";
  case MIROp::Call:
    return "call";
  case MIROp::InlineAssembly:
    return "asm";
  case MIROp::Unreachable:
    return "unreachable";
  }
  return "nop";
}

HIROp HIRBuilder::operation_for(NodeKind kind) {
  switch (kind) {
  case NodeKind::Program:
    return HIROp::Module;
  case NodeKind::Function:
    return HIROp::Function;
  case NodeKind::Parameter:
    return HIROp::Parameter;
  case NodeKind::Block:
  case NodeKind::BugemoanBlock:
    return HIROp::Block;
  case NodeKind::Declaration:
  case NodeKind::TypeAlias:
  case NodeKind::Using:
    return HIROp::Declaration;
  case NodeKind::If:
    return HIROp::If;
  case NodeKind::While:
  case NodeKind::DoWhile:
  case NodeKind::For:
  case NodeKind::RangeFor:
    return HIROp::Loop;
  case NodeKind::Switch:
    return HIROp::Switch;
  case NodeKind::Match:
    return HIROp::Switch;
  case NodeKind::Case:
  case NodeKind::Default:
    return HIROp::Case;
  case NodeKind::Return:
  case NodeKind::CoroutineReturn:
  case NodeKind::CoroutineYield:
    return HIROp::Return;
  case NodeKind::Break:
    return HIROp::Break;
  case NodeKind::Continue:
    return HIROp::Continue;
  case NodeKind::Goto:
    return HIROp::Goto;
  case NodeKind::Label:
    return HIROp::Label;
  case NodeKind::Throw:
    return HIROp::Throw;
  case NodeKind::Try:
  case NodeKind::Catch:
    return HIROp::Try;
  case NodeKind::AsmStatement:
    return HIROp::Assembly;
  case NodeKind::Namespace:
    return HIROp::Namespace;
  case NodeKind::Class:
  case NodeKind::Struct:
  case NodeKind::Union:
    return HIROp::Record;
  case NodeKind::Enum:
    return HIROp::Enumeration;
  case NodeKind::Preprocessor:
    return HIROp::Directive;
  default:
    return HIROp::Expression;
  }
}

HIRNode HIRBuilder::lower(const THIRNode &node) {
  HIRNode result{next_id_++,
                 operation_for(node.source_kind),
                 node.location,
                 node.text,
                 {}};
  if (result.text.empty())
    result.text = node_name(node.source_kind);
  result.children.reserve(node.children.size());
  for (const auto &child : node.children)
    result.children.push_back(lower(child));
  return result;
}

HIRModule HIRBuilder::build(const THIRModule &program) {
  next_id_ = 0;
  HIRModule result;
  result.source_name = program.source_name;
  result.declarations.reserve(program.declarations.size());
  for (const auto &child : program.declarations)
    result.declarations.push_back(lower(child));
  result.node_count = next_id_;
  return result;
}

void HIRModule::print(std::ostream &output) const {
  output << "hir.module \"" << source_name << "\" nodes " << node_count << '\n';
  for (const auto &declaration : declarations)
    print_hir_node(declaration, output, 1);
}

MIRBlock &MIRBuilder::make_block(std::string name) {
  function_->blocks.push_back({next_block_++, std::move(name), {}, false});
  return function_->blocks.back();
}

void MIRBuilder::select(std::size_t block) {
  block_ = &function_->blocks.at(block);
}

void MIRBuilder::emit(MIROp operation, const HIRNode &source,
                      std::vector<std::string> operands, std::string result) {
  block_->instructions.push_back(
      {operation, std::move(result), std::move(operands), source.location});
  if (operation == MIROp::Return || operation == MIROp::Throw ||
      operation == MIROp::Jump || operation == MIROp::Branch ||
      operation == MIROp::BranchIf || operation == MIROp::Switch ||
      operation == MIROp::Unreachable)
    block_->terminated = true;
}

void MIRBuilder::lower_if(const HIRNode &node) {
  const std::size_t origin_id = block_->id;
  const std::size_t then_id = make_block("if.then").id;
  const std::size_t else_id = make_block("if.else").id;
  const std::size_t merge_id = make_block("if.end").id;
  select(origin_id);
  const std::string condition =
      node.children.empty() ? "<missing>" : operand_text(node.children[0]);
  emit(MIROp::BranchIf, node,
       {condition, "bb" + std::to_string(then_id),
        "bb" + std::to_string(else_id)});
  select(then_id);
  if (node.children.size() > 1)
    lower_node(node.children[1]);
  if (!block_->terminated)
    emit(MIROp::Jump, node, {"bb" + std::to_string(merge_id)});
  select(else_id);
  if (node.children.size() > 2)
    lower_node(node.children[2]);
  if (!block_->terminated)
    emit(MIROp::Jump, node, {"bb" + std::to_string(merge_id)});
  select(merge_id);
}

void MIRBuilder::lower_loop(const HIRNode &node) {
  const std::size_t origin_id = block_->id;
  const std::size_t header_id = make_block("loop.header").id;
  const std::size_t body_id = make_block("loop.body").id;
  const std::size_t exit_id = make_block("loop.exit").id;
  select(origin_id);
  emit(MIROp::Jump, node, {"bb" + std::to_string(header_id)});
  select(header_id);
  emit(MIROp::BranchIf, node,
       {"loop.condition", "bb" + std::to_string(body_id),
        "bb" + std::to_string(exit_id)});
  select(body_id);
  for (const auto &child : node.children)
    lower_node(child);
  if (!block_->terminated)
    emit(MIROp::Jump, node, {"bb" + std::to_string(header_id)});
  select(exit_id);
}

void MIRBuilder::lower_switch(const HIRNode &node) {
  const std::size_t origin_id = block_->id;
  const HIRNode *body = node.children.size() > 1 ? &node.children[1] : nullptr;
  struct Arm {
    const HIRNode *label;
    std::size_t block;
    std::size_t begin;
    std::size_t end;
  };
  std::vector<Arm> arms;
  const std::size_t exit_id = make_block("switch.exit").id;
  if (body) {
    for (std::size_t index = 0; index < body->children.size(); ++index) {
      if (body->children[index].operation != HIROp::Case)
        continue;
      if (!arms.empty())
        arms.back().end = index;
      const std::size_t arm_id = make_block("switch.case").id;
      arms.push_back(
          {&body->children[index], arm_id, index + 1, body->children.size()});
    }
  }
  select(origin_id);
  std::vector<std::string> operands = {
      node.children.empty() ? "<missing>" : operand_text(node.children[0])};
  for (const auto &arm : arms) {
    const std::string label = arm.label->children.empty()
                                  ? "default"
                                  : operand_text(arm.label->children.front());
    operands.push_back(label + ":bb" + std::to_string(arm.block));
  }
  operands.push_back("exit:bb" + std::to_string(exit_id));
  emit(MIROp::Switch, node, std::move(operands));
  for (const auto &arm : arms) {
    select(arm.block);
    for (std::size_t index = arm.begin; body && index < arm.end; ++index)
      lower_node(body->children[index]);
    if (!block_->terminated)
      emit(MIROp::Jump, *arm.label, {"bb" + std::to_string(exit_id)});
  }
  select(exit_id);
}

void MIRBuilder::lower_node(const HIRNode &node) {
  if (block_->terminated)
    return;
  switch (node.operation) {
  case HIROp::Block:
    for (const auto &child : node.children)
      lower_node(child);
    break;
  case HIROp::Declaration:
    emit(MIROp::Declare, node, {node.text});
    for (const auto &child : node.children)
      lower_node(child);
    break;
  case HIROp::Expression:
    emit(MIROp::Evaluate, node, {operand_text(node)},
         "%t" + std::to_string(next_temporary_++));
    break;
  case HIROp::If:
    lower_if(node);
    break;
  case HIROp::Loop:
    lower_loop(node);
    break;
  case HIROp::Switch:
    lower_switch(node);
    break;
  case HIROp::Return:
    emit(MIROp::Return, node,
         node.children.empty()
             ? std::vector<std::string>{}
             : std::vector<std::string>{operand_text(node.children[0])});
    break;
  case HIROp::Goto:
    emit(MIROp::Jump, node, {node.text});
    break;
  case HIROp::Throw:
    emit(MIROp::Throw, node, {node.text});
    break;
  case HIROp::Assembly:
    emit(MIROp::InlineAssembly, node, {node.text});
    break;
  default:
    for (const auto &child : node.children)
      lower_node(child);
    break;
  }
}

void MIRBuilder::lower_function(const HIRNode &node, MIRModule &module) {
  module.functions.push_back({node.text, {}});
  function_ = &module.functions.back();
  next_block_ = 0;
  next_temporary_ = 0;
  make_block("entry");
  select(0);
  for (const auto &child : node.children)
    if (child.operation != HIROp::Parameter)
      lower_node(child);
  if (!block_->terminated)
    emit(MIROp::Return, node);
}

MIRModule MIRBuilder::build(const HIRModule &hir) {
  MIRModule result;
  result.source_name = hir.source_name;
  for (const auto &declaration : hir.declarations) {
    if (declaration.operation == HIROp::Function)
      lower_function(declaration, result);
    else if (declaration.operation == HIROp::Declaration)
      result.globals.push_back(declaration.text);
  }
  function_ = nullptr;
  block_ = nullptr;
  return result;
}

void MIRModule::print(std::ostream &output) const {
  output << "mir.module \"" << source_name << "\"\n";
  for (const auto &global : globals)
    output << "  global " << global << '\n';
  for (const auto &function : functions) {
    output << "  function \"" << function.signature << "\" {\n";
    for (const auto &block : function.blocks) {
      output << "    bb" << block.id << '.' << block.name << ":\n";
      for (const auto &instruction : block.instructions) {
        output << "      ";
        if (!instruction.result.empty())
          output << instruction.result << " = ";
        output << mir_name(instruction.operation);
        for (const auto &operand : instruction.operands)
          output << ' ' << operand;
        output << " at " << instruction.location.line << ':'
               << instruction.location.column << '\n';
      }
    }
    output << "  }\n";
  }
}

bool MIRModule::verify(std::string &error) const {
  for (const auto &function : functions) {
    if (function.signature.empty()) {
      error = "MIR function has no signature";
      return false;
    }
    if (function.blocks.empty()) {
      error = "MIR function '" + function.signature + "' has no entry block";
      return false;
    }
    for (std::size_t index = 0; index < function.blocks.size(); ++index) {
      const auto &block = function.blocks[index];
      if (block.id != index) {
        error = "MIR block identifiers are not dense in '" +
                function.signature + "'";
        return false;
      }
      if (!block.terminated) {
        error = "MIR block bb" + std::to_string(block.id) + " in '" +
                function.signature + "' has no terminator";
        return false;
      }
    }
  }
  return true;
}
} // namespace csp
