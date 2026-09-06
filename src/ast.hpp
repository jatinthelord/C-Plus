#pragma once
#include "token.hpp"
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace csp {
struct Node;

struct SourceRange {
  Location begin;
  Location end;

  bool valid() const { return !begin.file.empty(); }
};

enum class Access { Public, Protected, Private };
enum class StorageClass {
  None,
  Auto,
  Register,
  Static,
  Extern,
  ThreadLocal,
  Mutable
};
enum class FunctionSpecifier {
  None,
  Inline,
  Virtual,
  Explicit,
  Constexpr,
  Consteval,
  Constinit
};
enum class CvQualifier : unsigned {
  None = 0,
  Const = 1,
  Volatile = 2,
  Restrict = 4
};
enum class ReferenceKind { None, LValue, RValue };
enum class TypeKind {
  Invalid,
  Void,
  Boolean,
  Character,
  SignedInteger,
  UnsignedInteger,
  FloatingPoint,
  UnicodeCharacter,
  Named,
  Auto,
  Decltype,
  Pointer,
  Reference,
  Array,
  Function,
  Vector,
  TemplateSpecialization
};

struct Attribute {
  std::string scope;
  std::string name;
  std::vector<std::string> arguments;
  SourceRange range;
};

struct Type {
  TypeKind kind = TypeKind::Invalid;
  std::string name;
  unsigned bit_width = 0;
  CvQualifier qualifiers = CvQualifier::None;
  ReferenceKind reference = ReferenceKind::None;
  std::shared_ptr<Type> element;
  std::vector<std::shared_ptr<Type>> parameters;
  std::vector<std::shared_ptr<Type>> template_arguments;
  std::unique_ptr<Node> array_size;
  bool variadic = false;
  bool noexcept_function = false;

  bool is_valid() const { return kind != TypeKind::Invalid; }
  bool is_integer() const {
    return kind == TypeKind::SignedInteger ||
           kind == TypeKind::UnsignedInteger || kind == TypeKind::Character ||
           kind == TypeKind::UnicodeCharacter || kind == TypeKind::Boolean;
  }
  bool is_arithmetic() const {
    return is_integer() || kind == TypeKind::FloatingPoint;
  }
  bool is_pointer_like() const {
    return kind == TypeKind::Pointer || kind == TypeKind::Reference ||
           kind == TypeKind::Array;
  }
};

enum class UnaryOperator {
  Plus,
  Negate,
  LogicalNot,
  BitwiseNot,
  Dereference,
  AddressOf,
  PreIncrement,
  PreDecrement,
  Sizeof,
  Alignof,
  Noexcept
};

enum class BinaryOperator {
  Multiply,
  Divide,
  Remainder,
  Add,
  Subtract,
  ShiftLeft,
  ShiftRight,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  Equal,
  NotEqual,
  BitwiseAnd,
  BitwiseXor,
  BitwiseOr,
  LogicalAnd,
  LogicalOr,
  Comma
};

enum class AssignmentOperator {
  Assign,
  AddAssign,
  SubtractAssign,
  MultiplyAssign,
  DivideAssign,
  RemainderAssign,
  ShiftLeftAssign,
  ShiftRightAssign,
  AndAssign,
  XorAssign,
  OrAssign
};

enum class LiteralKind {
  Boolean,
  NullPointer,
  Integer,
  FloatingPoint,
  Character,
  String,
  UserDefined
};

enum class CastKind { CStyle, Static, Dynamic, Const, Reinterpret, Functional };

struct DeclarationInfo {
  std::string name;
  std::shared_ptr<Type> type;
  StorageClass storage = StorageClass::None;
  std::vector<Attribute> attributes;
  SourceRange range;
  bool exported = false;
  bool gpu_device = false;
  bool gpu_host = false;
  bool gpu_global = false;
  bool gpu_shared = false;
  bool gpu_constant = false;
};

struct FunctionInfo : DeclarationInfo {
  std::vector<DeclarationInfo> parameters;
  std::vector<std::string> template_parameters;
  FunctionSpecifier specifier = FunctionSpecifier::None;
  bool variadic = false;
  bool deleted = false;
  bool defaulted = false;
  bool pure_virtual = false;
};

struct KernelConfiguration {
  std::unique_ptr<Node> grid;
  std::unique_ptr<Node> block;
  std::unique_ptr<Node> shared_memory;
  std::unique_ptr<Node> stream;
};

enum class NodeKind {
  Program,
  Preprocessor,
  Function,
  Declaration,
  Parameter,
  Block,
  If,
  While,
  For,
  Return,
  Goto,
  Label,
  Break,
  Continue,
  ExpressionStatement,
  Identifier,
  Literal,
  UnaryExpression,
  BinaryExpression,
  AssignmentExpression,
  CallExpression,
  KernelLaunchExpression,
  IndexExpression,
  MemberExpression,
  CastExpression,
  InitializerList,
  ConditionalExpression,
  PostfixExpression,
  DoWhile,
  Switch,
  Case,
  Default,
  RangeFor,
  Try,
  Catch,
  Throw,
  Namespace,
  Class,
  Struct,
  Union,
  Enum,
  Template,
  Using,
  TypeAlias,
  StaticAssert,
  EmptyStatement,
  NewExpression,
  DeleteExpression,
  LambdaExpression,
  SizeofExpression,
  AlignofExpression,
  TypeidExpression,
  ThisExpression,
  NullptrExpression,
  CoroutineReturn,
  CoroutineYield,
  AsmStatement,
  BugemoanBlock,
  Match
};
inline const char *node_name(NodeKind k) {
  static const char *n[] = {"Program",
                            "Preprocessor",
                            "Function",
                            "Declaration",
                            "Parameter",
                            "Block",
                            "If",
                            "While",
                            "For",
                            "Return",
                            "Goto",
                            "Label",
                            "Break",
                            "Continue",
                            "ExpressionStatement",
                            "Identifier",
                            "Literal",
                            "UnaryExpression",
                            "BinaryExpression",
                            "AssignmentExpression",
                            "CallExpression",
                            "KernelLaunchExpression",
                            "IndexExpression",
                            "MemberExpression",
                            "CastExpression",
                            "InitializerList",
                            "ConditionalExpression",
                            "PostfixExpression",
                            "DoWhile",
                            "Switch",
                            "Case",
                            "Default",
                            "RangeFor",
                            "Try",
                            "Catch",
                            "Throw",
                            "Namespace",
                            "Class",
                            "Struct",
                            "Union",
                            "Enum",
                            "Template",
                            "Using",
                            "TypeAlias",
                            "StaticAssert",
                            "EmptyStatement",
                            "NewExpression",
                            "DeleteExpression",
                            "LambdaExpression",
                            "SizeofExpression",
                            "AlignofExpression",
                            "TypeidExpression",
                            "ThisExpression",
                            "NullptrExpression",
                            "CoroutineReturn",
                            "CoroutineYield",
                            "AsmStatement",
                            "BugemoanBlock",
                            "Match"};
  return n[(int)k];
}
struct Node {
  NodeKind kind;
  Location location;
  std::string value;
  std::vector<std::unique_ptr<Node>> children;
  Node(NodeKind k, Location l, std::string v = {})
      : kind(k), location(std::move(l)), value(std::move(v)) {}
  void dump(std::ostream &out, int depth = 0) const {
    out << std::string(depth * 2, ' ') << node_name(kind);
    if (!value.empty())
      out << " [" << value << "]";
    out << " @" << location.line << ":" << location.column << "\n";
    for (auto &c : children)
      c->dump(out, depth + 1);
  }

  Node *add(std::unique_ptr<Node> child) {
    Node *result = child.get();
    children.push_back(std::move(child));
    return result;
  }

  template <typename... Args> Node *emplace(Args &&...args) {
    return add(std::make_unique<Node>(std::forward<Args>(args)...));
  }

  bool is(NodeKind expected) const { return kind == expected; }

  bool is_expression() const {
    switch (kind) {
    case NodeKind::Identifier:
    case NodeKind::Literal:
    case NodeKind::UnaryExpression:
    case NodeKind::BinaryExpression:
    case NodeKind::AssignmentExpression:
    case NodeKind::CallExpression:
    case NodeKind::KernelLaunchExpression:
    case NodeKind::IndexExpression:
    case NodeKind::MemberExpression:
    case NodeKind::CastExpression:
    case NodeKind::InitializerList:
    case NodeKind::ConditionalExpression:
    case NodeKind::PostfixExpression:
    case NodeKind::NewExpression:
    case NodeKind::DeleteExpression:
    case NodeKind::LambdaExpression:
    case NodeKind::SizeofExpression:
    case NodeKind::AlignofExpression:
    case NodeKind::TypeidExpression:
    case NodeKind::ThisExpression:
    case NodeKind::NullptrExpression:
      return true;
    default:
      return false;
    }
  }

  bool is_statement() const {
    switch (kind) {
    case NodeKind::Block:
    case NodeKind::If:
    case NodeKind::While:
    case NodeKind::For:
    case NodeKind::Return:
    case NodeKind::Goto:
    case NodeKind::Label:
    case NodeKind::Break:
    case NodeKind::Continue:
    case NodeKind::ExpressionStatement:
    case NodeKind::Declaration:
    case NodeKind::DoWhile:
    case NodeKind::Switch:
    case NodeKind::Case:
    case NodeKind::Default:
    case NodeKind::Try:
    case NodeKind::Catch:
    case NodeKind::Throw:
    case NodeKind::StaticAssert:
    case NodeKind::EmptyStatement:
    case NodeKind::CoroutineReturn:
    case NodeKind::BugemoanBlock:
    case NodeKind::Match:
    case NodeKind::CoroutineYield:
    case NodeKind::AsmStatement:
      return true;
    default:
      return false;
    }
  }

  const Node *child(std::size_t index) const {
    return index < children.size() ? children[index].get() : nullptr;
  }

  Node *child(std::size_t index) {
    return index < children.size() ? children[index].get() : nullptr;
  }

  std::size_t subtree_size() const {
    std::size_t result = 1;
    for (const auto &item : children)
      result += item->subtree_size();
    return result;
  }

  std::size_t subtree_depth() const {
    std::size_t result = 1;
    for (const auto &item : children)
      result = std::max(result, 1 + item->subtree_depth());
    return result;
  }

  const Node *find_first(NodeKind wanted) const {
    if (kind == wanted)
      return this;
    for (const auto &item : children)
      if (const Node *found = item->find_first(wanted))
        return found;
    return nullptr;
  }

  std::vector<const Node *> find_all(NodeKind wanted) const {
    std::vector<const Node *> result;
    if (kind == wanted)
      result.push_back(this);
    for (const auto &item : children) {
      auto nested = item->find_all(wanted);
      result.insert(result.end(), nested.begin(), nested.end());
    }
    return result;
  }

  std::unique_ptr<Node> clone() const {
    auto result = std::make_unique<Node>(kind, location, value);
    for (const auto &item : children)
      result->children.push_back(item->clone());
    return result;
  }
};

class AstVisitor {
public:
  virtual ~AstVisitor() = default;

  virtual bool enter(Node &) { return true; }
  virtual void leave(Node &) {}

  void walk(Node &node) {
    if (!enter(node))
      return;
    for (auto &child : node.children)
      walk(*child);
    leave(node);
  }
};

class ConstAstVisitor {
public:
  virtual ~ConstAstVisitor() = default;

  virtual bool enter(const Node &) { return true; }
  virtual void leave(const Node &) {}

  void walk(const Node &node) {
    if (!enter(node))
      return;
    for (const auto &child : node.children)
      walk(*child);
    leave(node);
  }
};

class AstStatistics final : public ConstAstVisitor {
  std::vector<std::size_t> counts_;
  std::size_t maximum_depth_ = 0;
  std::size_t current_depth_ = 0;

public:
  AstStatistics()
      : counts_(static_cast<std::size_t>(NodeKind::AsmStatement) + 1) {}

  bool enter(const Node &node) override {
    ++current_depth_;
    maximum_depth_ = std::max(maximum_depth_, current_depth_);
    ++counts_[static_cast<std::size_t>(node.kind)];
    return true;
  }

  void leave(const Node &) override { --current_depth_; }

  std::size_t count(NodeKind kind) const {
    return counts_[static_cast<std::size_t>(kind)];
  }

  std::size_t maximum_depth() const { return maximum_depth_; }

  std::size_t total() const {
    std::size_t result = 0;
    for (std::size_t count : counts_)
      result += count;
    return result;
  }
};

inline std::string escape_json(const std::string &input) {
  std::string result;
  result.reserve(input.size() + 8);
  for (unsigned char character : input) {
    switch (character) {
    case '"':
      result += "\\\"";
      break;
    case '\\':
      result += "\\\\";
      break;
    case '\b':
      result += "\\b";
      break;
    case '\f':
      result += "\\f";
      break;
    case '\n':
      result += "\\n";
      break;
    case '\r':
      result += "\\r";
      break;
    case '\t':
      result += "\\t";
      break;
    default:
      if (character < 0x20) {
        static const char *digits = "0123456789abcdef";
        result += "\\u00";
        result += digits[character >> 4];
        result += digits[character & 15];
      } else {
        result += static_cast<char>(character);
      }
    }
  }
  return result;
}

inline void write_json(const Node &node, std::ostream &output, int indent = 0) {
  std::string margin(static_cast<std::size_t>(indent), ' ');
  output << margin << "{\n";
  output << margin << "  \"kind\": \"" << node_name(node.kind) << "\",\n";
  output << margin << "  \"value\": \"" << escape_json(node.value) << "\",\n";
  output << margin << "  \"location\": {\"file\": \""
         << escape_json(node.location.file)
         << "\", \"line\": " << node.location.line
         << ", \"column\": " << node.location.column << "},\n";
  output << margin << "  \"children\": [";
  if (!node.children.empty())
    output << "\n";
  for (std::size_t index = 0; index < node.children.size(); ++index) {
    write_json(*node.children[index], output, indent + 4);
    if (index + 1 != node.children.size())
      output << ',';
    output << '\n';
  }
  output << margin << "  ]\n";
  output << margin << '}';
}

inline const char *type_kind_name(TypeKind kind) {
  switch (kind) {
  case TypeKind::Invalid:
    return "invalid";
  case TypeKind::Void:
    return "void";
  case TypeKind::Boolean:
    return "bool";
  case TypeKind::Character:
    return "char";
  case TypeKind::SignedInteger:
    return "signed integer";
  case TypeKind::UnsignedInteger:
    return "unsigned integer";
  case TypeKind::FloatingPoint:
    return "floating point";
  case TypeKind::UnicodeCharacter:
    return "unicode character";
  case TypeKind::Named:
    return "named";
  case TypeKind::Auto:
    return "auto";
  case TypeKind::Decltype:
    return "decltype";
  case TypeKind::Pointer:
    return "pointer";
  case TypeKind::Reference:
    return "reference";
  case TypeKind::Array:
    return "array";
  case TypeKind::Function:
    return "function";
  case TypeKind::Vector:
    return "vector";
  case TypeKind::TemplateSpecialization:
    return "template specialization";
  }
  return "unknown";
}

inline const char *unary_operator_spelling(UnaryOperator operation) {
  switch (operation) {
  case UnaryOperator::Plus:
    return "+";
  case UnaryOperator::Negate:
    return "-";
  case UnaryOperator::LogicalNot:
    return "!";
  case UnaryOperator::BitwiseNot:
    return "~";
  case UnaryOperator::Dereference:
    return "*";
  case UnaryOperator::AddressOf:
    return "&";
  case UnaryOperator::PreIncrement:
    return "++";
  case UnaryOperator::PreDecrement:
    return "--";
  case UnaryOperator::Sizeof:
    return "sizeof";
  case UnaryOperator::Alignof:
    return "alignof";
  case UnaryOperator::Noexcept:
    return "noexcept";
  }
  return "?";
}

inline const char *binary_operator_spelling(BinaryOperator operation) {
  switch (operation) {
  case BinaryOperator::Multiply:
    return "*";
  case BinaryOperator::Divide:
    return "/";
  case BinaryOperator::Remainder:
    return "%";
  case BinaryOperator::Add:
    return "+";
  case BinaryOperator::Subtract:
    return "-";
  case BinaryOperator::ShiftLeft:
    return "<<";
  case BinaryOperator::ShiftRight:
    return ">>";
  case BinaryOperator::Less:
    return "<";
  case BinaryOperator::LessEqual:
    return "<=";
  case BinaryOperator::Greater:
    return ">";
  case BinaryOperator::GreaterEqual:
    return ">=";
  case BinaryOperator::Equal:
    return "==";
  case BinaryOperator::NotEqual:
    return "!=";
  case BinaryOperator::BitwiseAnd:
    return "&";
  case BinaryOperator::BitwiseXor:
    return "^";
  case BinaryOperator::BitwiseOr:
    return "|";
  case BinaryOperator::LogicalAnd:
    return "&&";
  case BinaryOperator::LogicalOr:
    return "||";
  case BinaryOperator::Comma:
    return ",";
  }
  return "?";
}

inline const char *assignment_operator_spelling(AssignmentOperator operation) {
  switch (operation) {
  case AssignmentOperator::Assign:
    return "=";
  case AssignmentOperator::AddAssign:
    return "+=";
  case AssignmentOperator::SubtractAssign:
    return "-=";
  case AssignmentOperator::MultiplyAssign:
    return "*=";
  case AssignmentOperator::DivideAssign:
    return "/=";
  case AssignmentOperator::RemainderAssign:
    return "%=";
  case AssignmentOperator::ShiftLeftAssign:
    return "<<=";
  case AssignmentOperator::ShiftRightAssign:
    return ">>=";
  case AssignmentOperator::AndAssign:
    return "&=";
  case AssignmentOperator::XorAssign:
    return "^=";
  case AssignmentOperator::OrAssign:
    return "|=";
  }
  return "?";
}

inline bool is_assignment_operator(const std::string &spelling) {
  return spelling == "=" || spelling == "+=" || spelling == "-=" ||
         spelling == "*=" || spelling == "/=" || spelling == "%=" ||
         spelling == "<<=" || spelling == ">>=" || spelling == "&=" ||
         spelling == "^=" || spelling == "|=";
}

inline bool is_comparison_operator(const std::string &spelling) {
  return spelling == "==" || spelling == "!=" || spelling == "<" ||
         spelling == "<=" || spelling == ">" || spelling == ">=";
}

inline bool is_short_circuit_operator(const std::string &spelling) {
  return spelling == "&&" || spelling == "||";
}

inline bool is_gpu_node(const Node &node) {
  return node.kind == NodeKind::KernelLaunchExpression ||
         node.value == "__global__" || node.value == "__device__" ||
         node.value == "__host__" || node.value == "__shared__";
}

inline std::string type_spelling(const Type &type) {
  std::string result;
  if ((static_cast<unsigned>(type.qualifiers) &
       static_cast<unsigned>(CvQualifier::Const)) != 0)
    result += "const ";
  if ((static_cast<unsigned>(type.qualifiers) &
       static_cast<unsigned>(CvQualifier::Volatile)) != 0)
    result += "volatile ";

  switch (type.kind) {
  case TypeKind::Invalid:
    result += "<invalid>";
    break;
  case TypeKind::Void:
    result += "void";
    break;
  case TypeKind::Boolean:
    result += "bool";
    break;
  case TypeKind::Character:
    result += type.name.empty() ? "char" : type.name;
    break;
  case TypeKind::SignedInteger:
    result += type.name.empty() ? "int" : type.name;
    break;
  case TypeKind::UnsignedInteger:
    result += type.name.empty() ? "unsigned int" : type.name;
    break;
  case TypeKind::FloatingPoint:
    result += type.name.empty() ? "double" : type.name;
    break;
  case TypeKind::UnicodeCharacter:
    result += type.name.empty() ? "unichar" : type.name;
    break;
  case TypeKind::Named:
  case TypeKind::Decltype:
    result += type.name;
    break;
  case TypeKind::Auto:
    result += "auto";
    break;
  case TypeKind::Pointer:
    result += type.element ? type_spelling(*type.element) : "<missing>";
    result += '*';
    break;
  case TypeKind::Reference:
    result += type.element ? type_spelling(*type.element) : "<missing>";
    result += type.reference == ReferenceKind::RValue ? "&&" : "&";
    break;
  case TypeKind::Array:
    result += type.element ? type_spelling(*type.element) : "<missing>";
    result += "[]";
    break;
  case TypeKind::Function:
    result += type.element ? type_spelling(*type.element) : "void";
    result += " (";
    for (std::size_t index = 0; index < type.parameters.size(); ++index) {
      if (index)
        result += ", ";
      result += type_spelling(*type.parameters[index]);
    }
    if (type.variadic)
      result += type.parameters.empty() ? "..." : ", ...";
    result += ')';
    break;
  case TypeKind::Vector:
    result += "vector<";
    result += type.element ? type_spelling(*type.element) : "<missing>";
    result += '>';
    break;
  case TypeKind::TemplateSpecialization:
    result += type.name + '<';
    for (std::size_t index = 0; index < type.template_arguments.size();
         ++index) {
      if (index)
        result += ", ";
      result += type_spelling(*type.template_arguments[index]);
    }
    result += '>';
    break;
  }
  return result;
}
} // namespace csp
