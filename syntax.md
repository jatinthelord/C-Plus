# C+ language reference

Copyright (c) 2026 CSP Foundation. SPDX-License-Identifier: MIT.

This document describes syntax accepted by the current C+ lexer and parser.
The canonical source extension is `.csp`; CP ASM uses `.cpsm`. C+ keeps the
familiar C-family declaration model and adds safety, tooling, GPU, matching,
and explicit low-level facilities. The canonical program entry point is:

```c++
#include <cpstream>

int main(void) {
    printc("Hello from C+\n");
    return 0;
}
```

Compile and run it with:

```console
cspc build hello.csp -O2 -o hello
./hello
```

On Windows, run `hello.exe` instead of `./hello`.

## Lexical structure

C+ source is UTF-8. A UTF-8 byte-order mark is accepted at the beginning of a
translation unit. Invalid, overlong, surrogate, truncated, and out-of-range
UTF-8 sequences in identifiers are diagnosed by the lexer.

Whitespace separates tokens but otherwise has no semantic meaning. A source
file may begin with a tool shebang:

```c++
#!/usr/bin/env cspc
int main(void) { return 0; }
```

Line comments and nested block comments are accepted:

```c++
// A line comment.

/* An outer comment.
   /* A nested comment. */
   The outer comment continues here. */
```

Preprocessor directives begin with `#` at the start of a logical line.
Backslash-newline continues a directive:

```c++
#include <cpstream>
#define CSP_SUM(a, b) \
    ((a) + (b))
```

Identifiers begin with `_`, an ASCII letter, or a valid non-ASCII UTF-8 code
point. Following characters may also contain decimal digits. Identifiers are
case-sensitive: `item`, `Item`, and `ITEM` name different entities.

## Literals

Integer literals support decimal, hexadecimal, binary, and octal notation:

```c++
int decimal = 42;
int hex = 0x2A;
int binary = 0b101010;
int octal = 052;
unsigned long separated = 1'000'000UL;
```

Digit separators must appear between digits. A separator cannot follow a base
prefix, repeat without a digit, or terminate a number.

Floating-point literals support decimal exponents and hexadecimal binary
exponents:

```c++
double pi = 3.141592653589793;
float small = 1.25e-4F;
double hex_float = 0x1.8p+2;
```

Quoted strings, characters, encoding prefixes, escapes, and raw strings are
recognized as complete lexer tokens:

```c++
char newline = '\n';
const char *plain = "C+";
const char8_t *utf8 = u8"systems";
const wchar_t *wide = L"wide";
const char *raw = R"tag(first line
second line)tag";
```

Supported escapes include `\\`, `\'`, `\"`, `\?`, `\a`, `\b`, `\f`, `\n`,
`\r`, `\t`, `\v`, octal escapes, `\xNN`, `\uNNNN`, and `\UNNNNNNNN`.
Raw-string delimiters may contain at most 16 valid delimiter characters.

Boolean and null-pointer literals are `true`, `false`, and `nullptr`.

## Fundamental types and declarations

Recognized fundamental spellings include:

| Family | Types |
| --- | --- |
| Empty | `void` |
| Boolean | `bool` |
| Characters | `char`, `wchar_t`, `char8_t`, `char16_t`, `char32_t`, `unichar` |
| Signed integers | `short`, `int`, `long`, `signed` combinations |
| Unsigned integers | `unsigned` combinations, `u8`, `u16`, `u32`, `u64`, `u128` |
| Floating point | `float`, `double`, `long double` |
| Size | `size_t` |
| Inferred | `auto`, `let` |

`goto` is a control-flow keyword, not a data type.

```c++
int count = 2;
float ratio = 2.01F;
const u64 mask = 0xFF00'0000ULL;
let inferred = 42;
let mut changing = 1;
```

Variables declared with `let` are immutable unless `mut` is present. The
semantic pass rejects assignment, prefix mutation, and postfix mutation of an
immutable binding.

Arrays use standard declarator syntax and braced initialization:

```c++
int values[3] = {1, 3, 7};
char name[8] = {'c', 's', 'p', '\0'};
int matrix[2][3] = {{1, 2, 3}, {4, 5, 6}};
```

Pointers and references preserve their type information:

```c++
int value = 21;
int *pointer = &value;
const int *read_only = pointer;
void *opaque = static_cast<void *>(pointer);
int &reference = value;
int &&temporary = 42;
```

Dereference with `*`, take an address with `&`, and access pointer members with
`->`. A plain integer is not a pointer, and a pointer must not be printed as an
integer. Use `%p` and cast to `void *` when required by the formatting API.

```c++
printc("value=%d address=%p\n", *pointer, static_cast<void *>(pointer));
```

## Expressions and precedence

Primary expressions are identifiers, literals, `this`, `nullptr`, parenthesized
expressions, and braced initializer lists. Postfix operations are applied
before unary and binary operations.

From highest to lowest binding strength, the implemented expression grammar is:

| Level | Operators or forms | Association |
| ---: | --- | --- |
| 15 | `()`, `[]`, `.`, `->`, postfix `++`, postfix `--`, kernel `<<< >>>` | left |
| 14 | prefix `+ - ! ~ * & ++ --`, `sizeof`, `alignof`, `typeid`, `new`, `delete` | right |
| 12 | `* / %` | left |
| 11 | `+ -` | left |
| 10 | `<< >>` | left |
| 9 | `< > <= >=` | left |
| 8 | `== !=` | left |
| 7 | `&` | left |
| 6 | `^` | left |
| 5 | `|` | left |
| 4 | `&&` | left |
| 3 | `||` | left |
| 2 | `?:` | right |
| 1 | `= += -= *= /= %= <<= >>= &= ^= |=` | right |

Examples:

```c++
int arithmetic = 2 + 3 * 4;
bool range = lower <= value && value < upper;
flags |= permission_write;
index <<= 1;
const char *label = ready ? "ready" : "waiting";
```

Calls and indexing accept comma-separated expressions:

```c++
int result = sum(left, right);
int element = values[index];
object.member = pointer->member;
```

Allocation and destruction syntax is explicit:

```c++
Widget *one = new Widget();
Widget *many = new Widget[capacity];
delete one;
delete[] many;
```

Compile-time queries use either a parenthesized operand or a prefix operand:

```c++
size_t bytes = sizeof(value);
size_t alignment = alignof(Widget);
auto identity = typeid(value);
```

## Statements and control flow

A block encloses zero or more statements. An empty statement is a single `;`.

```c++
{
    int local = 1;
    local += 2;
}
;
```

Conditional execution uses `if` and optional `else`:

```c++
if (value < 0) {
    printc("negative\n");
} else if (value == 0) {
    printc("zero\n");
} else {
    printc("positive\n");
}
```

The canonical counting loop is the ISO C-family form:

```c++
for (int i = 0; i < 50; ++i) {
    printc("%d\n", i);
}
```

The parser also preserves C+ shorthand loop headers for later lowering. Use the
canonical semicolon form in portable source.

```c++
while (connected) {
    poll();
}

do {
    retry();
} while (!ready);
```

`break` exits a loop or switch. `continue` begins the next loop iteration.

Switch labels are expressions followed by a colon. Only one `default` is
permitted, and duplicate constant cases are rejected.

```c++
switch (command) {
case command_start:
    start();
    break;
case command_stop:
    stop();
    break;
default:
    reject();
    break;
}
```

Labels and jumps use standard spelling. A jump cannot cross a function
boundary.

```c++
goto cleanup;

cleanup:
release_resources();
return 0;
```

`Goto` is accepted as a compatibility spelling, but new code should use
lowercase `goto`.

Functions return with `return`; coroutines use `co_return` and `co_yield`:

```c++
int square(int value) {
    return value * value;
}
```

Exceptions use `try`, one or more `catch` handlers, and `throw`:

```c++
try {
    connect();
} catch (const network_error &error) {
    log(error);
} catch (...) {
    throw;
}
```

An inline assembly statement contains a balanced operand group:

```c++
asm("pause");
```

Prefer CP ASM for substantial assembly because `.cpsm` receives dedicated
lexing, parsing, validation, and target-aware emission.

---


The canonical C+ program entry point remains C-style:
The canonical C+ program entry point remains C-style:

```c++
int main(void) {
    return 0;
}
```
int main(void) {
    return 0;
}
```

## Functions and callable types

A function declaration consists of a return type, name, parameter list,
optional qualifiers, and either a body or `;`.

```c++
int add(int left, int right);

int add(int left, int right) {
    return left + right;
}
```

Use `void` for a parameterless function rather than inventing a `main fn`
form. The entry point is always written `int main(void)`.

Default arguments belong in the declaration:

```c++
int open_port(int port = 8080);
```

Variadic declarations use `...`:

```c++
int log_values(const char *format, ...);
```

Function pointers use a parenthesized pointer declarator:

```c++
int (*operation)(int, int) = &add;
int answer = operation(20, 22);
```

The library wrapper provides a more explicit spelling:

```c++
cp::function_pointer<int(int, int)> operation = &add;
```

Supported function suffixes are preserved for semantic analysis and backend
lowering. They include cv/ref qualifiers, `noexcept`, trailing return types,
`requires`, `override`, `final`, CUDA launch bounds, deletion, defaulting, and
pure-virtual declarations.

```c++
class Interface {
public:
    virtual int read(void *data, size_t size) noexcept = 0;
    virtual ~Interface() = default;
};

auto magnitude(Point point) noexcept -> double {
    return sqrt(point.x * point.x + point.y * point.y);
}
```

Overloads may share a name when their parameter lists differ:

```c++
int convert(int value);
double convert(double value);
```

The semantic pass rejects conflicting declarations rather than silently
choosing one.

## Namespaces and aliases

Namespaces may be named, nested with `::`, or anonymous:

```c++
namespace csp::net {
    int connect(const char *address);
}

namespace {
    int translation_unit_private = 1;
}
```

Bring a namespace into lookup or declare a type alias with `using`:

```c++
using namespace csp::net;
using byte_count = u64;
```

Prefer qualified names in public headers to keep dependencies explicit.

## Structures, classes, and unions

Records may be declared without a body or defined with members:

```c++
struct ForwardDeclared;

struct Point {
    double x;
    double y;
};
```

Classes support access sections and base clauses:

```c++
class BufferedStream : public Stream {
public:
    explicit BufferedStream(size_t capacity);
    size_t read(void *destination, size_t count) override;

protected:
    void refill();

private:
    char *buffer_;
    size_t capacity_;
};
```

The access labels are `public:`, `protected:`, and `private:`. Base clauses may
carry access and `virtual` specifiers and are resolved in semantic analysis.

Unions share storage among their fields:

```c++
union Word {
    u32 value;
    u8 bytes[4];
};
```

Use a tagged union when the active alternative must be tracked safely:

```c++
cp::tagged_union<int, const char *> result =
    cp::tagged_union<int, const char *>::first(200);
```

Layout attributes document ABI intent:

```c++
[[repr(C)]]
struct PacketHeader {
    u16 kind;
    u16 flags;
    u32 length;
};

alignas(64) struct CacheLine {
    u8 bytes[64];
};
```

Never assume a packed layout unless a supported representation attribute or
backend ABI contract states it.

## Enumerations

Plain, scoped, and explicitly represented enums are accepted:

```c++
enum Color {
    red,
    green = 4,
    blue
};

enum class Permission : u32 {
    none = 0,
    read = 1u << 0,
    write = 1u << 1,
    execute = 1u << 2
};
```

A trailing comma is valid. Enumerator initializers are expressions. Scoped
enumerators are referenced as `Permission::read`.

Bit flags combine independent enum values:

```c++
cp::bit_flags<Permission> permissions(Permission::read);
permissions.set(Permission::write);

if (permissions.test(Permission::read)) {
    read_file();
}
```

## Templates, generics, and concepts

Templates retain their complete parameter spelling in the AST and attach to
the following declaration:

```c++
template <typename T>
T maximum(T left, T right) {
    return left < right ? right : left;
}
```

Non-type parameters are accepted:

```c++
template <typename T, size_t N>
struct Buffer {
    T data[N];
};
```

Constrained templates use concepts and `requires` clauses:

```c++
template <typename T>
concept Arithmetic = requires(T left, T right) {
    left + right;
};

template <Arithmetic T>
T sum(T left, T right) requires Arithmetic<T> {
    return left + right;
}
```

Concrete generic instantiations are intended to be monomorphized. Each used
type combination receives a specialized implementation instead of mandatory
runtime type erasure.

## Lambda expressions

Lambdas consist of a capture list, optional parameters and suffixes, and a
required block body:

```c++
int offset = 4;
auto add_offset = [offset](int value) -> int {
    return value + offset;
};
```

Capture defaults and explicit reference captures are preserved:

```c++
auto read_all = [&]() { consume(buffer); };
auto update = [&state, limit](int value) mutable {
    state = minimum(value, limit);
};
```

The compiler treats the closure as a value. A capture does not imply a heap
allocation; escape analysis and backend lowering determine storage.

## Pattern matching

`match` uses case and default labels inside a block:

```c++
match (state) {
case State::idle:
    wait();
    break;
case State::running:
    execute();
    break;
case State::stopped:
    finish();
    break;
}
```

For a known enum domain, the typed representation checks exhaustiveness.
Duplicate arms are rejected. A `default` arm makes an otherwise incomplete
match exhaustive but may hide newly added enum values, so exhaustive explicit
arms are preferred for protocol and safety state machines.

## Compile-time evaluation

`constexpr`, `consteval`, `constinit`, and `static_assert` describe work that is
required or eligible to happen during compilation.

```c++
constexpr int factorial(int value) {
    return value <= 1 ? 1 : value * factorial(value - 1);
}

consteval u32 protocol_version() {
    return 2;
}

constinit u32 active_version = protocol_version();

static_assert(factorial(5) == 120, "CTFE result is incorrect");
```

The assertion message must be a string literal. A failed assertion is a
compile-time diagnostic, not a runtime branch.

## Ownership and safety syntax

Values are stored directly by default. Pointer and reference syntax must be
written explicitly. Resource-owning library values release their resources at
scope exit.

```c++
int read_configuration(void) {
    cp::owned_buffer bytes(4096);
    return parse(bytes.data(), bytes.size());
} // bytes is released here
```

Nullable values use an optional type rather than a sentinel pointer:

```c++
Option<int> maybe_port = read_port();

if (maybe_port.has_value()) {
    connect(maybe_port.value());
}
```

The semantic pass rejects direct dereference of an optional. Presence must be
checked or explicitly unwrapped through a checked API.

The `bugemoan` block is an opt-in boundary for expert low-level pointer work:

```c++
bugemoan {
    void *opaque = acquire_device_mapping();
    u32 *registers = reinterpret_cast<u32 *>(opaque);
    registers[control_register] |= enabled_bit;
}
```

`bugemoan` does not turn the entire compiler or language into an unsafe mode.
Its scope is lexical, visible in the AST, and available to semantic checks,
auditing tools, and code review. It is not required for ordinary pointers.

## GPU declarations and kernel launch

CUDA-compatible declaration specifiers are tokens in the language:

- `__global__` declares a kernel entry point.
- `__host__` declares host availability.
- `__device__` declares device availability.
- `__shared__` declares block-shared storage.
- `__constant__` declares constant device storage.
- `__managed__` declares managed storage.
- `__restrict__` promises non-aliasing within its contract.
- `__launch_bounds__` supplies launch constraints.
- `__noinline__` and `__forceinline__` control inlining intent.

Kernel launch syntax uses a grid and block, with optional shared-memory and
stream operands:

```c++
__global__ void add_scalar(const int *input, int *output, int scalar, int n) {
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index < n) {
        output[index] = input[index] + scalar;
    }
}

int main(void) {
    int blocks = (count + threads_per_block - 1) / threads_per_block;
    add_scalar<<<blocks, threads_per_block>>>(input, output, 5, count);
    return 0;
}
```

The extended launch form is:

```c++
kernel<<<grid, block, shared_bytes, stream>>>(arguments);
```

Kernel launch nodes remain distinct in the AST, allowing the semantic pass to
validate arity and the backend to select CUDA-specific emission.

## Preprocessor and linkage

Common preprocessing forms are passed through as complete directives:

```c++
#pragma once
#include <cp/http.h>
#include "project/config.h"
#define CSP_FEATURE_HTTP 1

#if CSP_FEATURE_HTTP
int serve(void);
#endif
```

C ABI linkage makes interoperability explicit:

```c++
extern "C" int c_library_open(const char *path);
```

Public FFI structures should use a stable representation and fixed-width types.
Ownership of pointers crossing the boundary must be documented by the API.

## Formal grammar

The following EBNF describes the current parser surface. Semantic restrictions
such as type compatibility, name resolution, exhaustiveness, and legal jump
targets are checked after parsing.

```ebnf
translation-unit      = { top-level-declaration } , end-of-file ;

top-level-declaration = preprocessor-directive
                      | namespace-declaration
                      | record-declaration
                      | enum-declaration
                      | using-declaration
                      | template-declaration
                      | static-assertion
                      | external-declaration ;

namespace-declaration = "namespace" , [ qualified-name ] , "{"
                        , { top-level-declaration } , "}" ;

qualified-name        = identifier , { "::" , identifier } ;

record-declaration    = record-key , [ identifier ] , [ base-clause ]
                        , ( ";" | record-body , [ ";" ] ) ;

record-key            = "class" | "struct" | "union" ;

base-clause           = ":" , base-specifier , { "," , base-specifier } ;

base-specifier        = [ access-specifier ] , [ "virtual" ]
                        , qualified-name ;

access-specifier      = "public" | "protected" | "private" ;

record-body           = "{" , { access-label | top-level-declaration } , "}" ;

access-label          = access-specifier , ":" ;

enum-declaration      = "enum" , [ "class" | "struct" ] , [ identifier ]
                        , [ ":" , type-id ]
                        , ( ";" | enum-body , [ ";" ] ) ;

enum-body             = "{" , [ enumerator-list , [ "," ] ] , "}" ;

enumerator-list       = enumerator , { "," , enumerator } ;

enumerator            = identifier , [ "=" , expression ] ;

using-declaration     = "using" , ( "namespace" , qualified-name
                        | identifier , "=" , type-id ) , ";" ;

template-declaration  = "template" , "<" , template-parameter-list , ">"
                        , top-level-declaration ;

template-parameter-list = template-parameter
                          , { "," , template-parameter } ;

template-parameter    = ( "typename" | "class" ) , identifier
                      | declaration ;

external-declaration  = declaration , ";"
                      | function-declaration
                      | function-definition ;

function-declaration  = declaration-prefix , parameter-list
                        , { function-suffix } , [ "=" , function-ending ]
                        , ";" ;

function-definition   = declaration-prefix , parameter-list
                        , { function-suffix } , block ;

function-ending       = "default" | "delete" | integer-literal ;

parameter-list        = "(" , [ parameter , { "," , parameter } ] , ")" ;

parameter             = declaration | "..." ;

function-suffix       = "const" | "volatile" | "&" | "&&"
                      | "noexcept" , [ "(" , expression , ")" ]
                      | "override" | "final"
                      | "->" , type-id
                      | "requires" , expression ;

block                 = "{" , { statement } , "}" ;

statement             = block
                      | empty-statement
                      | declaration-statement
                      | expression-statement
                      | if-statement
                      | while-statement
                      | do-while-statement
                      | for-statement
                      | switch-statement
                      | match-statement
                      | case-label
                      | try-statement
                      | throw-statement
                      | return-statement
                      | coroutine-statement
                      | jump-statement
                      | label-statement
                      | static-assertion
                      | asm-statement
                      | bugemoan-statement ;

empty-statement       = ";" ;

declaration-statement = declaration , ";" ;

expression-statement  = expression , ";" ;

if-statement          = "if" , "(" , expression , ")" , statement
                        , [ "else" , statement ] ;

while-statement       = "while" , "(" , expression , ")" , statement ;

do-while-statement    = "do" , statement , "while" , "(" , expression
                        , ")" , ";" ;

for-statement         = "for" , "(" , for-init , ";" , [ expression ]
                        , ";" , [ expression ] , ")" , statement ;

for-init              = [ declaration | expression ] ;

switch-statement      = "switch" , "(" , expression , ")" , statement ;

match-statement       = "match" , "(" , expression , ")" , statement ;

case-label            = "case" , expression , ":" | "default" , ":" ;

try-statement         = "try" , block , catch-handler , { catch-handler } ;

catch-handler         = "catch" , "(" , catch-parameter , ")" , block ;

catch-parameter       = declaration | "..." ;

throw-statement       = "throw" , [ expression ] , ";" ;

return-statement      = "return" , [ expression ] , ";" ;

coroutine-statement   = ( "co_return" | "co_yield" )
                        , [ expression ] , ";" ;

jump-statement        = ( "break" | "continue" ) , ";"
                      | ( "goto" | "Goto" ) , identifier , ";" ;

label-statement       = identifier , ":" ;

static-assertion      = "static_assert" , "(" , expression
                        , [ "," , string-literal ] , ")" , ";" ;

asm-statement         = "asm" , "(" , balanced-token-sequence , ")" , ";" ;

bugemoan-statement    = "bugemoan" , block ;

expression            = assignment-expression
                      , [ "?" , expression , ":" , expression ] ;

assignment-expression = logical-or-expression
                      , [ assignment-operator , assignment-expression ] ;

assignment-operator   = "=" | "+=" | "-=" | "*=" | "/=" | "%="
                      | "<<=" | ">>=" | "&=" | "^=" | "|=" ;

logical-or-expression = logical-and-expression
                      , { "||" , logical-and-expression } ;

logical-and-expression = bitwise-or-expression
                       , { "&&" , bitwise-or-expression } ;

bitwise-or-expression = bitwise-xor-expression
                      , { "|" , bitwise-xor-expression } ;

bitwise-xor-expression = bitwise-and-expression
                       , { "^" , bitwise-and-expression } ;

bitwise-and-expression = equality-expression
                       , { "&" , equality-expression } ;

equality-expression   = relational-expression
                      , { ( "==" | "!=" ) , relational-expression } ;

relational-expression = shift-expression
                      , { ( "<" | ">" | "<=" | ">=" )
                        , shift-expression } ;

shift-expression      = additive-expression
                      , { ( "<<" | ">>" ) , additive-expression } ;

additive-expression   = multiplicative-expression
                      , { ( "+" | "-" ) , multiplicative-expression } ;

multiplicative-expression = prefix-expression
                          , { ( "*" | "/" | "%" )
                            , prefix-expression } ;

prefix-expression     = postfix-expression
                      | prefix-operator , prefix-expression
                      | "sizeof" , query-operand
                      | "alignof" , query-operand
                      | "typeid" , query-operand
                      | new-expression
                      | delete-expression
                      | lambda-expression ;

prefix-operator       = "+" | "-" | "!" | "~" | "*" | "&"
                      | "++" | "--" ;

query-operand         = "(" , expression , ")" | prefix-expression ;

postfix-expression    = primary-expression , { postfix-operation } ;

postfix-operation     = "(" , [ argument-list ] , ")"
                      | "[" , expression , "]"
                      | ( "." | "->" ) , identifier
                      | "++" | "--"
                      | kernel-configuration , "(" , [ argument-list ] , ")" ;

kernel-configuration  = "<<<" , expression , "," , expression
                        , [ "," , expression , [ "," , expression ] ]
                        , ">>>" ;

argument-list         = expression , { "," , expression } ;

primary-expression    = identifier
                      | literal
                      | "this"
                      | "nullptr"
                      | "(" , expression , ")"
                      | initializer-list ;

initializer-list      = "{" , [ expression , { "," , expression } ] , "}" ;

new-expression        = "new" , type-id
                      , [ "(" , [ argument-list ] , ")" ]
                      | "new" , type-id , "[" , expression , "]" ;

delete-expression     = "delete" , [ "[" , "]" ] , prefix-expression ;

lambda-expression     = "[" , capture-list , "]" , [ parameter-list ]
                        , { lambda-suffix } , block ;

capture-list          = balanced-token-sequence ;

lambda-suffix         = "mutable" | "constexpr" | "consteval"
                      | "noexcept" | "->" , type-id
                      | "requires" , expression ;

literal               = integer-literal | floating-literal | string-literal
                      | character-literal | "true" | "false" | "nullptr" ;
```

## Complete keyword inventory

The lexer recognizes the following reserved spellings. Some are parsed by the
frontend today; others are preserved so compatible backends and later semantic
stages never misclassify them as identifiers.

```text
alignas alignof and and_eq asm
atomic_cancel atomic_commit atomic_noexcept
auto bitand bitor bool bugemoan break
case catch char char8_t char16_t char32_t class
co_await co_return co_yield compl concept
const consteval constexpr constinit const_cast continue contract_assert
decltype default delete do double dynamic_cast
else enum explicit export extern
false float for friend
goto Goto
if inline int
let long
match mut mutable
namespace new noexcept not not_eq nullptr
operator or or_eq
private protected public
register reinterpret_cast requires return
short signed sizeof static static_assert static_cast struct switch synchronized
template this thread_local throw true try typedef typeid typename
u8 u16 u32 u64 u128 unichar union unsigned using
virtual void volatile wchar_t while xor xor_eq
__hopa__ __global__ __host__ __device__ __shared__ __constant__ __managed__
__restrict__ __launch_bounds__ __device_builtin__ __cudart_builtin__
__noinline__ __forceinline__
```

Alternative operator words have their standard meanings:

| Word | Operator |
| --- | --- |
| `and` | `&&` |
| `and_eq` | `&=` |
| `bitand` | `&` |
| `bitor` | `|` |
| `compl` | `~` |
| `not` | `!` |
| `not_eq` | `!=` |
| `or` | `||` |
| `or_eq` | `|=` |
| `xor` | `^` |
| `xor_eq` | `^=` |

## Compiler command syntax

The compiler accepts either a direct source invocation or a named command:

```console
cspc main.csp -O2 -o app
cspc build main.csp -O2 -o app
```

Inspection commands:

```console
cspc lex main.csp
cspc parse main.csp
cspc ast-json main.csp -o ast.json
cspc thir main.csp -o main.thir
cspc hir main.csp -o main.hir
cspc mir main.csp -o main.mir
cspc stats main.csp
cspc reflect main.csp -o reflection.json
```

Validation and developer commands:

```console
cspc check main.csp
cspc check main.csp --recover
cspc fmt main.csp
cspc lint main.csp
cspc doctor
cspc env
cspc targets
cspc features
cspc commands
cspc version
cspc help
```

Backend commands:

```console
cspc cpp main.csp -o main.cpp
cspc c main.csp -o main.c
cspc rust main.csp -o main.rs
cspc llvm main.csp -o main.ll
cspc asm main.csp -o main.s
cspc object main.csp -o main.o
cspc cuda kernel.csp -o kernel
```

Build profiles:

```console
cspc debug main.csp -o app
cspc release main.csp -o app
cspc fast main.csp -o app
cspc vectorized main.csp -o app
cspc safe main.csp -o app
cspc sanitize main.csp -o app
cspc warnings main.csp
cspc freestanding kernel.csp -o kernel
```

Important options:

| Option | Meaning |
| --- | --- |
| `-o`, `--output` | Set the output path |
| `-O0` through `-O3` | Select optimization level |
| `-g` | Emit debug information |
| `-I path` | Add an include search path |
| `-D name=value` | Define a preprocessing symbol |
| `--compiler path` | Select the native toolchain executable |
| `--emit-cpp path` | Preserve generated C++ |
| `--emit-c` | Select the C transpiler |
| `--emit-rust` | Select the Rust transpiler |
| `--emit-llvm` | Emit LLVM IR |
| `--emit-thir path` | Write typed high-level IR |
| `--emit-hir path` | Write high-level IR |
| `--emit-mir path` | Write mid-level IR |
| `--ast-json path` | Write a machine-readable AST |
| `--stdout` | Write selected textual output to stdout |
| `--check` | Stop after frontend validation |
| `--recover` | Continue after recoverable parse errors |
| `--target triple` | Set the backend target triple |
| `--cpu name` | Set the backend CPU |
| `--native` | Tune for the build machine |
| `--lto` | Enable full link-time optimization |
| `--thin-lto` | Enable thin link-time optimization |
| `--vectorize` | Enable vectorization |
| `--no-vectorize` | Disable vectorization |
| `--fast-math` | Permit non-strict floating-point transforms |
| `--performance` | Select the performance-oriented profile |
| `--rebuild` | Ignore an up-to-date output cache |
| `--freestanding` | Build without hosted assumptions |
| `--no-runtime` | Do not inject the C+ runtime prelude |
| `--cuda` | Select CUDA compilation |
| `--gpu-arch name` | Select GPU architecture, such as `sm_86` |
| `--run` | Run the executable after a successful build |
| `--dump-tokens` | Print lexer tokens |
| `--dump-ast` | Print the parsed AST |

`hopa` performs the self-compilation workflow. It is not a general-purpose
unsafe mode:

```console
cspc hopa selfhost/cspc.csp -o cspc-stage2
```

## Diagnostic contract

Diagnostics use file, line, and column locations:

```text
main.csp:12:9: error: assignment to immutable binding 'count'
main.csp:12:9: help: declare it with 'let mut count' if mutation is intended
```

Recovery mode synchronizes at statement boundaries and reports multiple safe,
independent errors in one run. It stops at the configured error limit rather
than flooding the terminal with cascading failures.

Common lexer errors include:

- invalid UTF-8 in an identifier;
- an unterminated block comment;
- an unterminated quoted or raw string;
- a newline inside an ordinary quoted literal;
- an empty character literal;
- a malformed universal character escape;
- a numeric prefix without digits;
- a misplaced digit separator;
- an exponent without decimal digits;
- an unexpected source byte.

Common parser errors include:

- a missing delimiter such as `)`, `]`, `}`, or `;`;
- an unterminated block, namespace, record, or parameter list;
- an invalid enumerator;
- a `try` statement without a `catch` handler;
- a non-string `static_assert` message;
- an invalid expression prefix;
- a declaration without a terminator or function body.

Common semantic errors include:

- duplicate symbols or incompatible overloads;
- an unknown identifier;
- assignment to an immutable binding;
- invalid optional dereference;
- `break` or `continue` outside a valid context;
- a jump across a function boundary;
- duplicate switch cases or defaults;
- a non-exhaustive match;
- a failing compile-time assertion;
- invalid kernel configuration.

## Performance contract

Source syntax does not guarantee a fixed runtime such as `98 us`. Runtime
depends on the algorithm, input, generated code, target CPU, operating system,
link mode, and measurement method. C+ performance claims are valid only when
the benchmark publishes all of those conditions.

For optimized native output use:

```console
cspc performance workload.csp --native --lto --vectorize -O3 -o workload
```

`--fast-math` may improve floating-point throughput, but it changes IEEE edge
case behavior and must be selected explicitly:

```console
cspc performance simulation.csp --native --lto --vectorize --fast-math -O3
```

Measure a built executable separately from compilation. Use multiple samples,
report median and dispersion, keep inputs identical, prevent dead-code
elimination, and compare equivalent algorithms. Microsecond results are
meaningful only for workloads large enough to rise above timer and process
startup noise.

## Conformance status

This reference distinguishes three levels:

- **Lexed** means the spelling is reserved and tokenized correctly.
- **Parsed** means the frontend creates a structured AST node.
- **Checked/lowered** means semantic validation and a backend contract exist.

Do not infer full runtime support merely because a keyword is lexed. The
compiler's regression suite is the authority for checked behavior. New syntax
must add a positive test, a relevant rejection test, and documentation here.
must add a positive test, a relevant rejection test, and documentation here.

## CP ASM formal reference

CP ASM source is line-oriented. The first `.target` directive selects either
`x86_64` or `aarch64`; omitting it selects the implementation default. Comments
begin with `;` or `//` and continue to the end of the line.

```ebnf
cpasm-file          = { cpasm-line } , end-of-file ;

cpasm-line          = [ directive | label | instruction ] , newline ;

directive           = directive-name , [ operand-list ] ;

directive-name      = ".target"
                    | ".section"
                    | ".global"
                    | ".extern"
                    | ".align"
                    | ".byte"
                    | ".word"
                    | ".dword"
                    | ".qword"
                    | ".ascii"
                    | ".asciz"
                    | ".const"
                    | ".proc"
                    | ".end" ;

label               = identifier , ":" ;

instruction         = identifier , [ operand-list ] ;

operand-list        = operand , { "," , operand } ;

operand             = { identifier | number | string | "[" | "]"
                      | "+" | "-" | "#" } ;

number              = decimal-number | hexadecimal-number | binary-number ;

string              = '"' , { string-character | escape-sequence } , '"' ;
```

### Target selection

```asm
.target x86_64
```

or:

```asm
.target aarch64
```

Exactly one architecture operand is required. A target mismatch must be
reported before invoking an external assembler.

### Sections

The portable section names are `text`, `data`, and `rodata`:

```asm
.section rodata
message:
    .asciz "Hello from CP ASM"

.section data
counter:
    .qword 0

.section text
```

The GNU emitter maps those names to the platform assembler section spelling.

### Symbol visibility

`.global` exports a symbol and `.extern` declares a symbol supplied by another
object or library:

```asm
.global main
.extern puts
```

Procedure shorthand combines a symbol boundary with executable code:

```asm
.proc main
    xor eax, eax
    ret
.end
```

`.proc` takes exactly one symbol name. `.end` takes no operands.

### Constants and alignment

Named assembler constants use `.const`:

```asm
.const buffer_size, 4096
.const enabled_bit, 0x20
```

Alignment is explicit:

```asm
.align 16
vector_table:
```

The requested alignment must be supported by the selected assembler and object
format. It does not allocate storage by itself.

### Data emission

Fixed-width integer directives are:

```asm
.byte 0x7f
.word 0x1234
.dword 0x12345678
.qword 0x123456789abcdef0
```

Multiple operands may be emitted on one line:

```asm
.byte 1, 2, 3, 4
.dword 10, 20, 30, 40
```

`.ascii` emits string bytes without an automatic terminator. `.asciz` appends a
zero byte:

```asm
.ascii "C+"
.asciz "terminated"
```

### x86-64 operands

x86-64 uses Intel destination-first operand order:

```asm
.target x86_64
.proc add_two
    mov eax, edi
    add eax, esi
    ret
.end
```

Memory operands are bracketed and may contain base, index, scale, and
displacement tokens supported by the downstream assembler:

```asm
    mov eax, [rdi]
    mov [rsi + 8], eax
    lea rax, [rdi + rcx]
```

Immediate operands are written as numbers. Labels can be branch operands:

```asm
    xor ecx, ecx
loop_start:
    add ecx, 1
    cmp ecx, 10
    jl loop_start
```

### AArch64 operands

AArch64 register and immediate conventions remain native:

```asm
.target aarch64
.proc add_two
    add w0, w0, w1
    ret
.end
```

The `#` token marks immediate values where the instruction syntax requires it:

```asm
    mov w0, #0
    add x1, x1, #16
```

Bracketed load/store operands are preserved:

```asm
    ldr w0, [x1]
    str w0, [x2, #4]
```

### CP ASM diagnostics

Each token carries a file, line, and column. The frontend reports:

- unknown directives;
- malformed `.target` declarations;
- unsupported architecture names;
- invalid `.proc` operands;
- missing commas between operands;
- unterminated strings;
- unexpected punctuation;
- labels with invalid names;
- source lines that are neither directives, labels, nor instructions.

The emitter validates directives again before producing GNU assembly. CP ASM
does not copy arbitrary input directly into an assembler process.

### Mixed C+ and CP ASM projects

Compile CP ASM into an object and link it with a C+ translation unit:

```console
cspc object fast_path.cpsm -o fast_path.o
cspc build main.csp fast_path.o -O3 -o application
```

Exported procedures must follow the platform ABI. Declare the matching symbol
in C+ with C linkage:

```c++
extern "C" int add_two(int left, int right);

int main(void) {
    return add_two(20, 22) == 42 ? 0 : 1;
}
```

Register preservation, stack alignment, parameter locations, return registers,
name decoration, and unwind information are the assembly author's
responsibility. Use a C+ wrapper to isolate target-specific procedures.

## Syntax contribution checklist

Before extending the language grammar:

1. Add the spelling to the lexer only if it is reserved.
2. Add a distinct AST node when the construct has distinct semantics.
3. Preserve source locations for the construct and its operands.
4. Define precedence and associativity for every new operator.
5. Add parser recovery boundaries around the construct.
6. Resolve names and types in the semantic pass.
7. Specify ownership, mutation, and control-flow behavior.
8. Define THIR, HIR, and MIR lowering where applicable.
9. Define backend behavior or emit a clear unsupported diagnostic.
10. Add valid syntax tests.
11. Add invalid syntax and semantic rejection tests.
12. Add formatter and linter handling.
13. Add editor highlighting and language-server handling.
14. Document the syntax and one complete example here.
15. Benchmark only when the change can affect performance.