#include "../src/diagnostic.hpp"
#include "../src/lexer.hpp"
#include "../src/lowering.hpp"
#include "../src/ir.hpp"
#include "../src/parser.hpp"
#include "../src/semantic.hpp"
#include "../src/transpiler.hpp"
#include "../src/thir.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

static void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

static std::unique_ptr<csp::Node> parse(const std::string &source) {
  csp::Lexer lexer(source, "test.csp");
  auto tokens = lexer.tokenize();
  csp::Parser parser(tokens);
  return parser.parse();
}

static void test_numeric_lexing() {
  csp::Lexer lexer("0 42 0xff 0b1010 1'000 2.5f 0x1.fp3", "numbers.csp");
  auto tokens = lexer.tokenize();
  expect(tokens.size() == 8, "numeric token count");
  expect(tokens[0].kind == csp::TokenKind::Integer, "zero is integer");
  expect(tokens[2].text == "0xff", "hexadecimal spelling");
  expect(tokens[3].text == "0b1010", "binary spelling");
  expect(tokens[5].kind == csp::TokenKind::Floating, "decimal float");
  expect(tokens[6].kind == csp::TokenKind::Floating, "hexadecimal float");
}

static void test_literals_and_comments() {
  const std::string source = R"(
    // ignored
    "normal\n" R"tag(raw text)tag" '\x41'
    /* nested /* extension */ comment */
  )";
  csp::Lexer lexer(source, "literals.csp");
  auto tokens = lexer.tokenize();
  expect(tokens.size() == 4, "literal token count");
  expect(tokens[0].kind == csp::TokenKind::String, "normal string");
  expect(tokens[1].kind == csp::TokenKind::String, "raw string");
  expect(tokens[2].kind == csp::TokenKind::Character, "character literal");
}

static void test_expression_precedence() {
  auto tree = parse("int main(){ result = a + b * c; return result; }");
  auto assignments = tree->find_all(csp::NodeKind::AssignmentExpression);
  auto products = tree->find_all(csp::NodeKind::BinaryExpression);
  expect(assignments.size() == 1, "one assignment expression");
  expect(products.size() == 2, "addition and multiplication nodes");
  expect(assignments[0]->value == "=", "assignment operator spelling");
}

static void test_all_assignment_operators() {
  auto tree = parse(R"(
    int main() {
      int value = 1;
      value += 2; value -= 1; value *= 3; value /= 2; value %= 2;
      value <<= 1; value >>= 1; value &= 7; value ^= 3; value |= 8;
      return value;
    }
  )");
  const auto assignments = tree->find_all(csp::NodeKind::AssignmentExpression);
  expect(assignments.size() == 10,
         "parser accepts the complete compound-assignment family");
}

static void test_csp_print_arguments() {
  auto tree = parse("int main(){ int n=3; printc(\"%d\"n); }");
  auto calls = tree->find_all(csp::NodeKind::CallExpression);
  expect(calls.size() == 1, "one call expression");
  expect(calls[0]->children.size() == 3,
         "C+ adjacent print argument becomes AST argument");
}

static void test_control_flow() {
  auto tree = parse(R"(
    int main() {
      if (ready) run(); else stop();
      while (active) tick();
      do tick(); while (active);
      switch (mode) { case 1: break; default: return 0; }
      try { work(); } catch (...) { recover(); }
      return 0;
    }
  )");
  expect(tree->find_all(csp::NodeKind::If).size() == 1, "if statement");
  expect(tree->find_all(csp::NodeKind::While).size() == 1, "while statement");
  expect(tree->find_all(csp::NodeKind::DoWhile).size() == 1,
         "do-while statement");
  expect(tree->find_all(csp::NodeKind::Switch).size() == 1, "switch statement");
  expect(tree->find_all(csp::NodeKind::Try).size() == 1, "try statement");
  expect(tree->find_all(csp::NodeKind::Catch).size() == 1, "catch handler");
}

static void test_kernel_launch() {
  auto tree = parse("int main(){ kernel<<<grid, block>>>(out, in, n); }");
  auto launches = tree->find_all(csp::NodeKind::KernelLaunchExpression);
  expect(launches.size() == 1, "CUDA launch AST node");
  expect(launches[0]->children.size() == 6,
         "callee, two configuration values, and three arguments");
}

static void test_lambda_expression() {
  auto tree = parse(R"(
    int main() {
      callback = [value, &state](int amount) mutable noexcept {
        state = value + amount;
        return state;
      };
      return callback(2);
    }
  )");
  auto lambdas = tree->find_all(csp::NodeKind::LambdaExpression);
  expect(lambdas.size() == 1, "lambda expression AST node");
  expect(lambdas[0]->value.find("value") != std::string::npos,
         "lambda capture spelling");
  expect(lambdas[0]->find_all(csp::NodeKind::Parameter).size() == 1,
         "lambda parameter AST node");
}

static void test_diagnostic_location() {
  try {
    csp::Lexer lexer("\n\n\"unterminated", "broken.csp");
    (void)lexer.tokenize();
    throw std::runtime_error("expected lexer failure");
  } catch (const csp::CompileError &error) {
    expect(error.location.file == "broken.csp", "diagnostic filename");
    expect(error.location.line == 3, "diagnostic line");
    expect(error.location.column == 1, "diagnostic column");
  }
}

static void test_parser_error_recovery() {
  const std::string source = R"(
    int first() {
      value = ;
      call(;
      return 1;
    }
    int second() {
      return 2;
    }
  )";
  csp::Lexer lexer(source, "recovery.csp");
  auto tokens = lexer.tokenize();
  csp::Parser parser(tokens);
  csp::DiagnosticEngine diagnostics(10);
  auto tree = parser.parse_recovering(diagnostics);
  expect(diagnostics.error_count() >= 2,
         "parser recovery reports multiple independent errors");
  expect(tree != nullptr, "parser recovery returns a partial AST");
  expect(tree->find_all(csp::NodeKind::Function).size() >= 1,
         "parser recovers far enough to parse a later function");
}

static void test_semantic_control_flow_checks() {
  auto tree = parse("int main(){ break; continue; return 0; }");
  csp::DiagnosticEngine diagnostics;
  csp::SemanticAnalyzer analyzer(diagnostics);
  expect(!analyzer.analyze(*tree), "invalid control flow fails semantics");
  expect(diagnostics.error_count() == 2,
         "break and continue outside loops both produce diagnostics");
}

static void test_semantic_goto_checks() {
  auto valid = parse("int main(){ goto done; done: return 0; }");
  csp::DiagnosticEngine valid_diagnostics;
  csp::SemanticAnalyzer valid_analyzer(valid_diagnostics);
  expect(valid_analyzer.analyze(*valid), "resolved goto passes semantics");

  auto invalid = parse("int main(){ goto missing; return 0; }");
  csp::DiagnosticEngine invalid_diagnostics;
  csp::SemanticAnalyzer invalid_analyzer(invalid_diagnostics);
  expect(!invalid_analyzer.analyze(*invalid),
         "unresolved goto fails semantics");
  expect(invalid_diagnostics.error_count() == 1,
         "unresolved goto emits one diagnostic");
}

static bool semantically_valid(const std::string &source,
                               std::size_t *errors = nullptr) {
  auto tree = parse(source);
  csp::DiagnosticEngine diagnostics(20);
  csp::SemanticAnalyzer analyzer(diagnostics);
  const bool valid = analyzer.analyze(*tree);
  if (errors)
    *errors = diagnostics.error_count();
  return valid;
}

static void test_semantic_function_boundaries() {
  expect(semantically_valid(R"(
    int first() { done: return 1; }
    int second() { done: return 2; }
  )"), "labels are local to their containing function");

  std::size_t errors = 0;
  expect(!semantically_valid(R"(
    int first() { goto done; }
    int second() { done: return 2; }
  )", &errors), "goto cannot target a label in another function");
  expect(errors == 1, "cross-function goto emits one diagnostic");

  expect(!semantically_valid(R"(
    int main() {
      while (running) {
        callback = []() { break; };
      }
      return 0;
    }
  )"), "loop control does not escape into a lambda body");
}

static void test_semantic_overloads() {
  expect(semantically_valid(R"(
    int convert(int value) { return value; }
    double convert(double value) { return value; }
  )"), "function overloads with different parameter types are valid");

  expect(semantically_valid(R"(
    int declared(int value);
    int declared(int value) { return value; }
  )"), "a prototype and matching definition are valid");

  expect(semantically_valid(R"(
    struct value {
      int get() { return 1; }
      int get() const { return 2; }
    };
  )"), "const and non-const member overloads are distinct");

  std::size_t errors = 0;
  expect(!semantically_valid(R"(
    int duplicate(int value) { return value; }
    int duplicate(int value) { return value + 1; }
  )", &errors), "duplicate function definitions are rejected");
  expect(errors == 1, "duplicate definition emits one diagnostic");

  errors = 0;
  expect(!semantically_valid(R"(
    int prototyped(int value);
    int prototyped(int value) { return value; }
    int prototyped(int value) { return value + 1; }
  )", &errors), "a prototype cannot hide duplicate definitions");
  expect(errors == 1,
         "duplicate definition after prototype emits one diagnostic");
}

static void test_semantic_switch_labels() {
  std::size_t errors = 0;
  expect(!semantically_valid("int main(){ case 1: return 0; }", &errors),
         "case outside switch is rejected");
  expect(errors == 1, "orphan case emits one diagnostic");

  expect(!semantically_valid(R"(
    int main() {
      switch (value) { default: break; default: break; }
      return 0;
    }
  )", &errors), "duplicate switch defaults are rejected");
  expect(errors == 1, "duplicate default emits one diagnostic");

  errors = 0;
  expect(!semantically_valid(R"(
    int main() {
      switch (value) { case 7: break; case 7: break; }
      return 0;
    }
  )", &errors), "duplicate literal case values are rejected");
  expect(errors == 1, "duplicate case value emits one diagnostic");
}

static void test_actionable_name_suggestion() {
  auto tree = parse("int main(){ int user_count = 3; return user_cout; }");
  csp::DiagnosticEngine diagnostics;
  csp::SemanticAnalyzer semantics(diagnostics);
  expect(semantics.analyze(*tree), "a spelling suggestion is non-fatal");
  expect(diagnostics.warning_count() == 1,
         "unknown nearby identifier emits one suggestion");
  expect(diagnostics.diagnostics()[0].message.find("user_count") !=
             std::string::npos,
         "suggestion names the closest declaration");
}

static void test_token_aware_lowering() {
  std::string source = R"CSP(#include <cpstream>
int main() {
  // printc("do not rewrite"value)
  const char *text = "for(int fake < 4, fake++;)";
  int values[3] = 1, 2, 3;
  printc("%d" values[0]);
  for (int i < 3, i++;)
    printc("%d"i);
})CSP";
  csp::Lexer lexer(source, "lowering.csp");
  auto tokens = lexer.tokenize();
  csp::Lowering lowering(source, tokens);
  std::string output = lowering.run();
  expect(output.find("#include <cpstream>") == std::string::npos,
         "runtime include removed structurally");
  expect(output.find("int values[3] = { 1, 2, 3}") != std::string::npos,
         "array shorthand lowered");
  expect(output.find("printc(\"%d\", values[0])") != std::string::npos,
         "print shorthand lowered");
  expect(output.find("for(int i = 0; i < 3; i++)") != std::string::npos,
         "short for loop lowered");
  expect(output.find("\"for(int fake < 4, fake++;)\"") != std::string::npos,
         "string contents are never rewritten");
  expect(output.find("// printc(\"do not rewrite\"value)") != std::string::npos,
         "comment contents are never rewritten");
}

static void test_c_and_rust_transpilers() {
  const std::string source =
      "int main(){ int value = 7; printc(\"value=%d\\n\"value); return 0; }";
  csp::Lexer lexer(source, "targets.csp");
  auto tokens = lexer.tokenize();
  csp::Parser parser(tokens);
  auto tree = parser.parse();
  csp::Lowering lowering(source, tokens);
  std::string lowered = lowering.run();
  csp::Lexer lowered_lexer(lowered, "targets.csp");
  auto lowered_tokens = lowered_lexer.tokenize();
  std::string c_source = csp::Transpiler::emit_c(lowered, lowered_tokens);
  std::string rust_source = csp::Transpiler::emit_rust(*tree);
  expect(c_source.find("#define printc printf") != std::string::npos,
         "C transpiler supplies print runtime");
  expect(c_source.find("printc(\"value=%d\\n\", value)") != std::string::npos,
         "C transpiler receives lowered C+ syntax");
  expect(rust_source.find("fn csp_main() -> i32") != std::string::npos,
         "Rust transpiler emits callable C+ main");
  expect(rust_source.find("unsafe { printf(") != std::string::npos,
         "Rust transpiler preserves printc semantics");
}

static void test_hir_and_mir_pipeline() {
  const std::string source = R"(
    int choose(int value) {
      auto floor = 4;
      if (value > 4) return value;
      return floor;
    }
  )";
  csp::Lexer lexer(source, "ir.csp");
  auto tokens = lexer.tokenize();
  csp::Parser parser(tokens);
  auto ast = parser.parse();
  csp::DiagnosticEngine diagnostics;
  csp::SemanticAnalyzer semantics(diagnostics);
  expect(semantics.analyze(*ast), "IR input passes semantic analysis");
  csp::HIRBuilder hir_builder;
  csp::THIRBuilder thir_builder;
  auto thir = thir_builder.build(*ast, "ir.csp");
  expect(thir.inferred_nodes == 1, "THIR infers an auto local at compile time");
  auto hir = hir_builder.build(thir);
  expect(hir.node_count >= 6, "HIR contains normalized source nodes");
  expect(hir.declarations.size() == 1 &&
             hir.declarations[0].operation == csp::HIROp::Function,
         "HIR recognizes function declarations");
  csp::MIRBuilder mir_builder;
  auto mir = mir_builder.build(hir);
  expect(mir.functions.size() == 1, "MIR contains one function");
  expect(mir.functions[0].blocks.size() >= 4,
         "MIR lowers conditional control flow into basic blocks");
  bool has_branch = false;
  for (const auto &block : mir.functions[0].blocks)
    for (const auto &instruction : block.instructions)
      has_branch |= instruction.operation == csp::MIROp::BranchIf;
  expect(has_branch, "MIR contains a conditional branch terminator");
}

int main() {
  try {
    test_numeric_lexing();
    test_literals_and_comments();
    test_expression_precedence();
    test_all_assignment_operators();
    test_csp_print_arguments();
    test_control_flow();
    test_kernel_launch();
    test_lambda_expression();
    test_diagnostic_location();
    test_parser_error_recovery();
    test_semantic_control_flow_checks();
    test_semantic_goto_checks();
    test_semantic_function_boundaries();
    test_semantic_overloads();
    test_semantic_switch_labels();
    test_actionable_name_suggestion();
    test_token_aware_lowering();
    test_c_and_rust_transpilers();
    test_hir_and_mir_pipeline();
    std::cout << "all C+ frontend tests passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "frontend test failure: " << error.what() << '\n';
    return 1;
  }
}
