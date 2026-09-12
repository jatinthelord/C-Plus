#include "semantic.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace csp {
void SemanticAnalyzer::enter_scope() {
  scopes_.emplace_back();
  variable_types_.emplace_back();
  optional_variables_.emplace_back();
  immutable_variables_.emplace_back();
}
void SemanticAnalyzer::leave_scope() {
  if (!scopes_.empty())
    scopes_.pop_back();
  if (!variable_types_.empty())
    variable_types_.pop_back();
  if (!optional_variables_.empty())
    optional_variables_.pop_back();
  if (!immutable_variables_.empty())
    immutable_variables_.pop_back();
}

std::string SemanticAnalyzer::declaration_name(const std::string &text) {
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

namespace {
std::string trim(std::string value) {
  while (!value.empty() &&
         std::isspace(static_cast<unsigned char>(value.front())))
    value.erase(value.begin());
  while (!value.empty() &&
         std::isspace(static_cast<unsigned char>(value.back())))
    value.pop_back();
  return value;
}

std::string parameter_type(std::string value) {
  if (const auto equals = value.find('='); equals != std::string::npos)
    value.resize(equals);
  value = trim(std::move(value));
  std::size_t end = value.size();
  while (end && std::isspace(static_cast<unsigned char>(value[end - 1])))
    --end;
  std::size_t begin = end;
  while (begin && (std::isalnum(static_cast<unsigned char>(value[begin - 1])) ||
                   value[begin - 1] == '_'))
    --begin;
  if (begin > 0 && begin < end)
    value.resize(begin);
  return trim(std::move(value));
}
} // namespace

bool SemanticAnalyzer::has_body(const Node &node) {
  for (const auto &child : node.children)
    if (child->kind == NodeKind::Block)
      return true;
  return false;
}

std::string SemanticAnalyzer::function_signature(const Node &node) {
  std::string result = declaration_name(node.value) + "(";
  bool first = true;
  for (const auto &child : node.children) {
    if (child->kind != NodeKind::Parameter)
      continue;
    if (!first)
      result += ',';
    result += parameter_type(child->value);
    first = false;
  }
  result += ')';
  if (const auto suffix = node.value.find('|'); suffix != std::string::npos)
    result += trim(node.value.substr(suffix));
  return result;
}

std::optional<std::string>
SemanticAnalyzer::constant_case_key(const Node &node) {
  if (node.kind == NodeKind::Literal)
    return node.value;
  if (node.kind == NodeKind::Identifier)
    return normalized_name(node.value);
  if (node.kind == NodeKind::MemberExpression && node.children.size() == 1) {
    auto owner = constant_case_key(*node.children[0]);
    if (owner)
      return normalized_name(*owner + node.value);
  }
  if (node.kind == NodeKind::UnaryExpression && node.children.size() == 1 &&
      (node.value == "+" || node.value == "-") &&
      node.children[0]->kind == NodeKind::Literal)
    return node.value + node.children[0]->value;
  return std::nullopt;
}

std::string SemanticAnalyzer::normalized_name(std::string text) {
  text.erase(std::remove_if(
                 text.begin(), text.end(),
                 [](unsigned char value) { return std::isspace(value) != 0; }),
             text.end());
  return text;
}

std::string SemanticAnalyzer::declaration_type(const std::string &text) {
  std::string cleaned = trim(text);
  const std::size_t space = cleaned.find_first_of(" \t*&");
  return space == std::string::npos ? std::string{} : cleaned.substr(0, space);
}

std::string SemanticAnalyzer::lookup_type(const std::string &name) const {
  for (auto scope = variable_types_.rbegin(); scope != variable_types_.rend();
       ++scope) {
    const auto found = scope->find(name);
    if (found != scope->end())
      return found->second;
  }
  return {};
}

bool SemanticAnalyzer::name_visible(const std::string &name) const {
  for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope)
    if (scope->contains(name))
      return true;
  return false;
}

bool SemanticAnalyzer::optional_visible(const std::string &name) const {
  for (auto scope = optional_variables_.rbegin();
       scope != optional_variables_.rend(); ++scope)
    if (scope->contains(name))
      return true;
  return false;
}

bool SemanticAnalyzer::immutable_visible(const std::string &name) const {
  for (auto scope = immutable_variables_.rbegin();
       scope != immutable_variables_.rend(); ++scope)
    if (scope->contains(name))
      return true;
  return false;
}

std::optional<std::string>
SemanticAnalyzer::closest_name(const std::string &name) const {
  auto distance = [](const std::string &left, const std::string &right) {
    std::vector<std::size_t> previous(right.size() + 1),
        current(right.size() + 1);
    for (std::size_t index = 0; index <= right.size(); ++index)
      previous[index] = index;
    for (std::size_t row = 1; row <= left.size(); ++row) {
      current[0] = row;
      for (std::size_t column = 1; column <= right.size(); ++column)
        current[column] =
            std::min({current[column - 1] + 1, previous[column] + 1,
                      previous[column - 1] +
                          (left[row - 1] == right[column - 1] ? 0 : 1)});
      previous.swap(current);
    }
    return previous.back();
  };
  std::optional<std::string> best;
  std::size_t best_distance = 3;
  for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope)
    for (const auto &[candidate, declarations] : *scope) {
      (void)declarations;
      const std::size_t candidate_distance = distance(name, candidate);
      if (candidate_distance < best_distance) {
        best = candidate;
        best_distance = candidate_distance;
      }
    }
  return best;
}

std::optional<long long> SemanticAnalyzer::evaluate_constant(const Node &node) {
  if (node.kind == NodeKind::Literal) {
    std::string spelling = node.value;
    spelling.erase(std::remove(spelling.begin(), spelling.end(), '\''),
                   spelling.end());
    if (spelling == "true")
      return 1;
    if (spelling == "false" || spelling == "nullptr")
      return 0;
    try {
      std::size_t consumed = 0;
      const long long value = std::stoll(spelling, &consumed, 0);
      if (consumed == spelling.size())
        return value;
    } catch (...) {
    }
    return std::nullopt;
  }
  if (node.kind == NodeKind::Identifier) {
    for (auto scope = ctfe_bindings_.rbegin(); scope != ctfe_bindings_.rend();
         ++scope) {
      const auto found = scope->find(node.value);
      if (found != scope->end())
        return found->second;
    }
    return std::nullopt;
  }
  if (node.kind == NodeKind::CallExpression && !node.children.empty() &&
      node.children.front()->kind == NodeKind::Identifier) {
    const auto function = ctfe_functions_.find(node.children.front()->value);
    if (function == ctfe_functions_.end())
      return std::nullopt;
    std::vector<long long> arguments;
    for (std::size_t index = 1; index < node.children.size(); ++index) {
      auto argument = evaluate_constant(*node.children[index]);
      if (!argument)
        return std::nullopt;
      arguments.push_back(*argument);
    }
    return evaluate_ctfe_function(*function->second, arguments);
  }
  if (node.kind == NodeKind::UnaryExpression && node.children.size() == 1) {
    auto value = evaluate_constant(*node.children[0]);
    if (!value)
      return std::nullopt;
    if (node.value == "+")
      return *value;
    if (node.value == "-")
      return -*value;
    if (node.value == "!")
      return !*value;
    if (node.value == "~")
      return ~*value;
  }
  if (node.kind == NodeKind::BinaryExpression && node.children.size() == 2) {
    auto left = evaluate_constant(*node.children[0]);
    auto right = evaluate_constant(*node.children[1]);
    if (!left || !right)
      return std::nullopt;
    const auto &op = node.value;
    if (op == "+")
      return *left + *right;
    if (op == "-")
      return *left - *right;
    if (op == "*")
      return *left * *right;
    if (op == "/")
      return *right ? std::optional<long long>(*left / *right) : std::nullopt;
    if (op == "%")
      return *right ? std::optional<long long>(*left % *right) : std::nullopt;
    if (op == "<<")
      return *left << *right;
    if (op == ">>")
      return *left >> *right;
    if (op == "<")
      return *left < *right;
    if (op == "<=")
      return *left <= *right;
    if (op == ">")
      return *left > *right;
    if (op == ">=")
      return *left >= *right;
    if (op == "==")
      return *left == *right;
    if (op == "!=")
      return *left != *right;
    if (op == "&")
      return *left & *right;
    if (op == "|")
      return *left | *right;
    if (op == "^")
      return *left ^ *right;
    if (op == "&&")
      return *left && *right;
    if (op == "||")
      return *left || *right;
  }
  return std::nullopt;
}

std::optional<long long> SemanticAnalyzer::evaluate_ctfe_function(
    const Node &function, const std::vector<long long> &arguments) {
  if (++ctfe_depth_ > 128) {
    --ctfe_depth_;
    return std::nullopt;
  }
  std::unordered_map<std::string, long long> bindings;
  std::size_t argument = 0;
  for (const auto &child : function.children) {
    if (child->kind != NodeKind::Parameter)
      continue;
    if (argument >= arguments.size()) {
      --ctfe_depth_;
      return std::nullopt;
    }
    bindings[declaration_name(child->value)] = arguments[argument++];
  }
  if (argument != arguments.size()) {
    --ctfe_depth_;
    return std::nullopt;
  }
  ctfe_bindings_.push_back(std::move(bindings));
  std::optional<long long> result;
  const auto find_return = [&](const auto &self,
                               const Node &node) -> const Node * {
    if (node.kind == NodeKind::Return && !node.children.empty())
      return &node;
    for (const auto &child : node.children)
      if (const Node *found = self(self, *child))
        return found;
    return nullptr;
  };
  if (const Node *returned = find_return(find_return, function))
    result = evaluate_constant(*returned->children.front());
  ctfe_bindings_.pop_back();
  --ctfe_depth_;
  return result;
}

void SemanticAnalyzer::check_exhaustive_match(
    const Node &node, const std::unordered_map<std::string, Location> &cases,
    bool has_default) {
  if (has_default || node.children.empty())
    return;
  const Node &condition = *node.children.front();
  if (condition.kind != NodeKind::Identifier) {
    diagnostics_.error(node.location,
                       "match exhaustiveness requires a named enum value");
    return;
  }
  const std::string type = lookup_type(condition.value);
  const auto enumeration = enum_members_.find(type);
  if (enumeration == enum_members_.end()) {
    diagnostics_.error(node.location,
                       "cannot determine enum type for exhaustive match");
    return;
  }
  std::vector<std::string> missing;
  for (const auto &member : enumeration->second) {
    const std::string qualified = normalized_name(type + "::" + member);
    if (!cases.contains(qualified) && !cases.contains(normalized_name(member)))
      missing.push_back(member);
  }
  if (!missing.empty()) {
    std::string message = "non-exhaustive match for '" + type + "'; missing ";
    for (std::size_t index = 0; index < missing.size(); ++index) {
      if (index)
        message += ", ";
      message += missing[index];
    }
    diagnostics_.error(node.location, std::move(message));
  }
}

void SemanticAnalyzer::declare_name(const std::string &name, const Node &node,
                                    bool allow_overload) {
  if (name.empty() || scopes_.empty())
    return;
  auto &entries = scopes_.back()[name];
  if (entries.empty()) {
    entries.push_back(&node);
    return;
  }
  if (allow_overload && node.kind == NodeKind::Function) {
    const std::string signature = function_signature(node);
    bool functions_only = true;
    bool matching_signature = false;
    bool existing_definition = false;
    for (const Node *existing : entries) {
      functions_only &= existing->kind == NodeKind::Function;
      if (existing->kind == NodeKind::Function &&
          function_signature(*existing) == signature) {
        matching_signature = true;
        existing_definition |= has_body(*existing);
      }
    }
    if (functions_only) {
      if (matching_signature && existing_definition && has_body(node)) {
        const Node *previous = nullptr;
        for (const Node *existing : entries)
          if (function_signature(*existing) == signature &&
              has_body(*existing)) {
            previous = existing;
            break;
          }
        auto &diagnostic = diagnostics_.error(
            node.location, "redefinition of function '" + signature + "'");
        if (previous)
          diagnostic.notes.push_back("previous definition is at line " +
                                     std::to_string(previous->location.line));
      }
      entries.push_back(&node);
      return;
    }
  }
  {
    auto &diagnostic =
        diagnostics_.error(node.location, "redeclaration of '" + name + "'");
    diagnostic.notes.push_back("previous declaration is at line " +
                               std::to_string(entries.front()->location.line));
  }
}

void SemanticAnalyzer::validate_gotos() {
  for (const auto &[target, location] : gotos_)
    if (!target.empty() && labels_.find(target) == labels_.end())
      diagnostics_.error(location,
                         "goto targets unknown label '" + target + "'");
}

void SemanticAnalyzer::visit_callable(const Node &node) {
  auto outer_labels = std::move(labels_);
  auto outer_gotos = std::move(gotos_);
  const unsigned outer_loops = loop_depth_;
  const unsigned outer_switches = switch_depth_;
  auto outer_defaults = std::move(switch_has_default_);
  auto outer_cases = std::move(switch_cases_);
  labels_.clear();
  gotos_.clear();
  loop_depth_ = 0;
  switch_depth_ = 0;
  switch_has_default_.clear();
  switch_cases_.clear();
  ++function_depth_;
  enter_scope();
  visit_children(node);
  leave_scope();
  --function_depth_;
  validate_gotos();
  labels_ = std::move(outer_labels);
  gotos_ = std::move(outer_gotos);
  loop_depth_ = outer_loops;
  switch_depth_ = outer_switches;
  switch_has_default_ = std::move(outer_defaults);
  switch_cases_ = std::move(outer_cases);
}

void SemanticAnalyzer::visit_children(const Node &node) {
  for (const auto &child : node.children)
    visit(*child);
}

void SemanticAnalyzer::visit(const Node &node) {
  switch (node.kind) {
  case NodeKind::Program:
  case NodeKind::Namespace:
  case NodeKind::Class:
  case NodeKind::Struct:
  case NodeKind::Union:
  case NodeKind::Block:
  case NodeKind::BugemoanBlock:
    enter_scope();
    visit_children(node);
    leave_scope();
    break;
  case NodeKind::Function:
    declare_name(declaration_name(node.value), node, true);
    if (node.value.find("constexpr") != std::string::npos ||
        node.value.find("consteval") != std::string::npos)
      ctfe_functions_[declaration_name(node.value)] = &node;
    visit_callable(node);
    break;
  case NodeKind::LambdaExpression:
    visit_callable(node);
    break;
  case NodeKind::Declaration:
    declare_name(declaration_name(node.value), node);
    if (!variable_types_.empty()) {
      const std::string name = declaration_name(node.value);
      const std::string type = declaration_type(node.value);
      if (!name.empty() && !type.empty())
        variable_types_.back()[name] = type;
      const auto name_position = node.value.find(name);
      if (!name.empty() && name_position != std::string::npos &&
          node.value.substr(0, name_position).find('?') != std::string::npos)
        optional_variables_.back().insert(name);
      if (!name.empty() && node.value.rfind("let ", 0) == 0 &&
          node.value.rfind("let mut ", 0) != 0)
        immutable_variables_.back().insert(name);
    }
    visit_children(node);
    break;
  case NodeKind::Parameter:
    declare_name(declaration_name(node.value), node);
    if (!variable_types_.empty()) {
      const std::string name = declaration_name(node.value);
      const std::string type = declaration_type(node.value);
      if (!name.empty() && !type.empty())
        variable_types_.back()[name] = type;
    }
    visit_children(node);
    break;
  case NodeKind::Enum: {
    std::string name = node.value;
    if (name.rfind("class ", 0) == 0)
      name.erase(0, 6);
    else if (name.rfind("struct ", 0) == 0)
      name.erase(0, 7);
    name = trim(name);
    declare_name(name, node);
    std::vector<std::string> members;
    for (const auto &child : node.children)
      if (child->kind == NodeKind::Declaration && child->children.size() <= 1)
        members.push_back(declaration_name(child->value));
    if (!name.empty())
      enum_members_[name] = std::move(members);
    visit_children(node);
    break;
  }
  case NodeKind::Identifier: {
    static const std::unordered_set<std::string> builtins = {
        "true",   "false",   "nullptr", "printc",
        "sizeof", "alignof", "typeid",  "this"};
    if (!builtins.contains(node.value) && !name_visible(node.value))
      if (auto suggestion = closest_name(node.value))
        diagnostics_.warning(node.location, "unknown name '" + node.value +
                                                "'; did you mean '" +
                                                *suggestion + "'?");
    break;
  }
  case NodeKind::UnaryExpression:
    if (node.value == "*" && node.children.size() == 1 &&
        node.children.front()->kind == NodeKind::Identifier &&
        optional_visible(node.children.front()->value))
      diagnostics_.error(node.location, "optional value requires an explicit "
                                        "presence check and .value() access");
    if ((node.value == "++" || node.value == "--") &&
        node.children.size() == 1 &&
        node.children.front()->kind == NodeKind::Identifier &&
        immutable_visible(node.children.front()->value))
      diagnostics_.error(node.location, "cannot modify immutable binding '" +
                                            node.children.front()->value + "'");
    visit_children(node);
    break;
  case NodeKind::PostfixExpression:
    if (node.children.size() == 1 &&
        node.children.front()->kind == NodeKind::Identifier &&
        immutable_visible(node.children.front()->value))
      diagnostics_.error(node.location, "cannot modify immutable binding '" +
                                            node.children.front()->value + "'");
    visit_children(node);
    break;
  case NodeKind::AssignmentExpression:
    if (!node.children.empty() &&
        node.children.front()->kind == NodeKind::Identifier &&
        immutable_visible(node.children.front()->value))
      diagnostics_.error(node.location, "cannot assign to immutable binding '" +
                                            node.children.front()->value + "'");
    visit_children(node);
    break;
  case NodeKind::Label:
    if (function_depth_ && !labels_.emplace(node.value, node.location).second)
      diagnostics_.error(node.location, "duplicate label '" + node.value + "'");
    break;
  case NodeKind::Goto:
    if (!function_depth_)
      diagnostics_.error(node.location, "goto is only valid inside a function");
    else
      gotos_.emplace_back(declaration_name(node.value), node.location);
    break;
  case NodeKind::For:
  case NodeKind::RangeFor:
  case NodeKind::While:
  case NodeKind::DoWhile:
    ++loop_depth_;
    visit_children(node);
    --loop_depth_;
    break;
  case NodeKind::Switch:
    ++switch_depth_;
    switch_has_default_.push_back(false);
    switch_cases_.emplace_back();
    visit_children(node);
    switch_cases_.pop_back();
    switch_has_default_.pop_back();
    --switch_depth_;
    break;
  case NodeKind::Match: {
    ++switch_depth_;
    switch_has_default_.push_back(false);
    switch_cases_.emplace_back();
    visit_children(node);
    const bool has_default = switch_has_default_.back();
    const auto cases = switch_cases_.back();
    check_exhaustive_match(node, cases, has_default);
    switch_cases_.pop_back();
    switch_has_default_.pop_back();
    --switch_depth_;
    break;
  }
  case NodeKind::Case:
    if (!switch_depth_)
      diagnostics_.error(node.location, "case is only valid inside a switch");
    else if (!node.children.empty()) {
      if (auto key = constant_case_key(*node.children.front())) {
        auto [existing, inserted] =
            switch_cases_.back().emplace(*key, node.location);
        if (!inserted) {
          auto &diagnostic = diagnostics_.error(
              node.location, "duplicate case value '" + *key + "'");
          diagnostic.notes.push_back("previous case is at line " +
                                     std::to_string(existing->second.line));
        }
      }
    }
    visit_children(node);
    break;
  case NodeKind::Default:
    if (!switch_depth_) {
      diagnostics_.error(node.location,
                         "default is only valid inside a switch");
    } else if (switch_has_default_.back()) {
      diagnostics_.error(node.location,
                         "switch cannot contain more than one default label");
    } else {
      switch_has_default_.back() = true;
    }
    break;
  case NodeKind::Break:
    if (!loop_depth_ && !switch_depth_)
      diagnostics_.error(node.location,
                         "break is only valid inside a loop or switch");
    break;
  case NodeKind::Continue:
    if (!loop_depth_)
      diagnostics_.error(node.location, "continue is only valid inside a loop");
    break;
  case NodeKind::Return:
  case NodeKind::CoroutineReturn:
  case NodeKind::CoroutineYield:
    if (!function_depth_)
      diagnostics_.error(node.location,
                         "return is only valid inside a function");
    visit_children(node);
    break;
  case NodeKind::Throw:
    if (!function_depth_)
      diagnostics_.error(node.location,
                         "throw is only valid inside a function");
    visit_children(node);
    break;
  case NodeKind::StaticAssert:
    if (!node.children.empty()) {
      const auto result = evaluate_constant(*node.children.front());
      if (result && !*result)
        diagnostics_.error(node.location,
                           node.value.empty()
                               ? "compile-time assertion failed"
                               : "compile-time assertion failed: " +
                                     node.value);
    }
    visit_children(node);
    break;
  case NodeKind::KernelLaunchExpression:
    if (node.children.size() < 3)
      diagnostics_.error(node.location,
                         "kernel launch requires grid and block dimensions");
    visit_children(node);
    break;
  default:
    visit_children(node);
    break;
  }
}

bool SemanticAnalyzer::analyze(const Node &program) {
  scopes_.clear();
  variable_types_.clear();
  optional_variables_.clear();
  immutable_variables_.clear();
  enum_members_.clear();
  ctfe_functions_.clear();
  ctfe_bindings_.clear();
  ctfe_depth_ = 0;
  labels_.clear();
  gotos_.clear();
  switch_has_default_.clear();
  switch_cases_.clear();
  visit(program);
  validate_gotos();
  return !diagnostics_.has_errors();
}
} // namespace csp
