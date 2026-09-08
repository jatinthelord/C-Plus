# c+ programming

print

#include <cpstream>

int main(void){
    printc("hello world");
    return 0;

}

variable

#include <cpstream>

int main(void){
    int num = 2;
    printc("hi %d"num);
    return 0;
    
}

arrays

#include <cpstream>

int main(void){
    int num[3] = 1, 3, 7;
    printc("hi %d"num);
    return 0;
    
}

data types

int, void, signed, unsigned, double, float, u16, u8, u32, u64, u128, goto, char, unichar, size_t


float variable example 

#include <cpstream>

int main(void){
    float num = 2.01;
    printc("hi %d"num);
    return 0;
    
}

for loops

#include <cpstream>

int main(void){
    for(int i < 50, i++;){
        printc("hello world");
    }
    return 0;
}
while loops

#include <cpstream>

int main(void){
    bool walk = false;
    while(bool walk == false){
        printc("you can't walk because the bool value is false");
    }
    return 0;
}

goto loops

#include <cpstream>

int main(void){
    Goto Label:
    printc("hello world");
    Goto Stop[label]
return 0;
}

vectors

#include <cpstream>

int main(void){
    int vec = {1.0f, 1.5f, 2.0f};
    printc("vector is %d");
    return 0;
}

pointers

#include <cpstream>

int main(void){
    int ptr;
    int a = 2;
    ptr = a;

    printc("%p"ptr);
    return 0;
}

void pointers, arrays GPU, loops in GPU, launch kernel you can learn from this, This is C+ own Syntax it matches to cuda but c+ is powerful that the cuda it's fast

#include <cpstream>

    __global__ int arr(int h_in, int h_out, int n){
        int i = threadIdx.x, BlockGrid.x, BlockDim.x;
        if(n < i){
            int == num;
        }
    }
  int main(void){
    int *arr
    int n = 5;
    size_t bytes = n * sizeof(int);

    int h_in[5] = {1,2,3,4,5};
    int h_out[5];

    int *d_in = nullptr;
    int *d_out = nullptr

    malloc((void**)&d_in, bytes);
    malloc((void**)&d_out, bytes);

    memcpy(d_in, h_in, bytes, cspHostToDevice);

    int ThreadPerBlock = 256;
    int blocksPerGrid = (n + ThreadPerBlock -1) / ThreadPerBlock;

    arr<<<blocksperGrid, ThreadPerBlock>>>(d_out, d_in, n);

    memcpy(d_out, h_out, bytes, memcpyDeviceToHost);

    for(int i = 0; i < n; i++){
        printc("hello\n", i, h_out[i]);
    }

    return(d_in);
    return(d_out);

    return 0;

}

---

# Standard library syntax

The new libraries use the `cp::` namespace. You can include their canonical
`cp/` header or use the shorter compatibility header:

```c++
#include <cp/map.h>  // canonical
#include <map.h>     // short form; provides the same API
```

Do not confuse `<map.h>` with the C++ `<map>` header. CSP's public container
types are named `cp::map`, `cp::set`, `cp::list`, and `cp::array`.

## Map

Maps store key/value pairs. `cp::map` keeps keys ordered, `cp::hash_map` uses a
hash table, and `cp::multimap` permits duplicate keys.

```c++
#include <map.h>
#include <cpstream>

int main(void) {
    cp::map<int, int> scores{{1, 40}, {2, 60}};
    scores[3] = 90;

    int *score = cp::find_value(scores, 2);
    if (score != nullptr) {
        printc("score=%d\n", *score);
    }

    cp::hash_map<int, int> cache;
    cache[10] = 100;
    return 0;
}
```

Available types and helpers:

- `cp::map<Key, Value>`
- `cp::multimap<Key, Value>`
- `cp::hash_map<Key, Value>`
- `cp::pmr_map<Key, Value>`
- `cp::find_value(map, key)` returns a pointer or `nullptr`
- `cp::value_copy(map, key)` returns an optional copy

## Set

Sets contain unique values. A multiset permits duplicates.

```c++
#include <set.h>

int main(void) {
    cp::set<int> left{1, 2, 3};
    cp::set<int> right{3, 4, 5};

    bool has_two = cp::contains(left, 2);
    cp::set<int> combined = cp::set_union(left, right);
    cp::set<int> common = cp::set_intersection(left, right);

    cp::hash_set<int> fast_lookup{10, 20, 30};
    return has_two && combined.size() == 5 && common.size() == 1 ? 0 : 1;
}
```

Available types: `cp::set`, `cp::multiset`, `cp::hash_set`, and `cp::pmr_set`.

## List

`cp::list` is a doubly linked list. `cp::forward_list` is a smaller singly
linked list.

```c++
#include <list.h>

int main(void) {
    cp::list<int> history{2, 3};
    cp::prepend(history, 1);
    cp::append(history, 4);

    int removed = (int)cp::erase_if(history, [](int value) {
        return value % 2 == 0;
    });
    return removed == 2 ? 0 : 1;
}
```

For allocator-aware programs, use `cp::pmr_list<T>`.

## Fixed arrays

The size of a `cp::array` is part of its type and is known during compilation.

```c++
#include <array.h>

int main(void) {
    cp::array<int, 3> numbers{2, 4, 8};
    auto generated = cp::make_array(1, 3, 7, 15);
    auto zeros = cp::filled_array<int, 8>(0);
    cp::byte_array<256> packet{};

    if (cp::in_bounds(numbers, 2)) {
        numbers[2] = generated[2];
    }
    return 0;
}
```

## Memory ownership

Use owners instead of manually calling `delete`. `cp::owner<T>` has one owner,
while `cp::shared_owner<T>` permits shared ownership. `cp::weak_owner<T>` can
observe a shared object without extending its lifetime.

```c++
#include <memory.h>

int main(void) {
    cp::owner<int> unique = cp::make_owner<int>(42);
    cp::shared_owner<int> shared = cp::make_shared_owner<int>(100);
    cp::weak_owner<int> weak = shared;
    cp::array_owner<int> buffer = cp::make_array<int>(128);

    int raw[3] = {1, 2, 3};
    auto values = cp::view(raw, 3);
    return *unique + *shared + values[0] == 143 ? 0 : 1;
}
```

Low-level helpers include `cp::copy_bytes`, `cp::zero_bytes`,
`cp::equal_bytes`, and `cp::align_up`.

## Construction and arena allocation

`cp::arena` quickly allocates many objects with one shared lifetime. Call an
object's destructor yourself before `release()` when that object owns resources.

```c++
#include <new.h>

struct point {
    int x;
    int y;
};

int main(void) {
    cp::arena temporary(8192);
    point *position = temporary.create<point>(point{4, 9});
    int answer = position->x + position->y;
    temporary.release();
    return answer == 13 ? 0 : 1;
}
```

For caller-provided storage, use `cp::construct<T>(storage, arguments...)` and
pair it with `cp::destroy(object)`.

## CSX Studio projects

`csxStudio.h` provides data structures for IDEs, build tools, and project
generators. It does not launch a process by itself; it produces compiler
arguments that tooling can inspect or execute.

```c++
#include <csxStudio.h>

int main(void) {
    csx::studio::project app{"server"};
    app.entry = "src/server.csp";
    app.output = "server.exe";
    app.build_target = csx::studio::target::native;
    app.optimize = csx::studio::optimization::maximum;
    app.include_paths.push_back(".csx/include");
    app.definitions.push_back("RELEASE=1");
    app.libraries.push_back("ws2_32");

    auto arguments = csx::studio::command(app);
    return arguments.empty() ? 1 : 0;
}
```

Build targets are `native`, `c`, `cpp`, `rust`, `llvm`, `assembly`, and `cuda`.
Optimization profiles are `debug`, `balanced`, `speed`, and `maximum`.

## Complete container example

The repository's `containers_demo.csp` combines maps, sets, lists, arrays,
owners, arena allocation, and CSX Studio in a program compiled by `cspc`.

```powershell
.\cspc.exe containers_demo.csp -O2 -o containers_demo.exe
.\containers_demo.exe
```

## Installing these libraries with CSX

Install only the containers:

```powershell
csx --prefix .csx install cp-containers
```

Install CSX Studio and its complete standard-library dependency graph:

```powershell
csx --prefix .csx install csx-studio
csx --prefix .csx verify
csx --prefix .csx installed
```

## Color, database, HTTP, and safety libraries

```c++
#include <cp/stdlib.h>

int main(void) {
    printc(cp::color::decorate("server ready\n", cp::color::basic::green));
    cp::http::request request{"GET", "/health", "example.com", ""};
    cp::db::memory_table services;
    services.insert({{"name", std::string{"api"}}, {"port", std::int64_t{8080}}});
    int value = 7;
    sg::not_null<int *> pointer(&value);
    int packet[3] = {1, 2, 3};
    bg::buffer guarded(packet);
    return guarded.at(2) == 3 && *pointer == 7 ? 0 : 1;
}
```

`sg` means safety guard: contracts, non-null pointers, checked numeric casts,
and scope-exit guards. `bg` means bounds guard: a sized buffer that throws on
an out-of-range `at()` access.

## Bugemoan pointer level (nightmare mode)

`bugemoan` is the hardest and most dangerous pointer level. It is intended for
allocators, kernels, operating systems, FFI, and runtime implementation. The
block marks unsafe code visibly; its capability key makes erased-pointer
operations explicit.

```c++
#include <buge.h>

int main(void) {
    bugemoan {
        auto access = bugemoan::enter_nightmare();
        auto memory = bugemoan::void_pointer::allocate(
            access, sizeof(int) * 4, alignof(int));
        int *numbers = memory.recover<int>(access);
        numbers[0] = 42;
        auto tail = memory.offset(access, sizeof(int));
        *tail.recover<int>(access) = 99;
    }
}
```

`void_pointer` is move-only and records address, byte extent, alignment, and
ownership. Recovery rejects null, undersized, and misaligned pointers; offset
rejects arithmetic outside the recorded region. `raw()` and `release()` exist
for ABI work but require the nightmare capability.

The compiler uses colored `note`, `warning`, and `error` labels on interactive
terminals. Set the standard `NO_COLOR` environment variable to disable them.
The `sg::finally` scope guard is inspired by D's scope-exit facilities: cleanup
is attached to lexical scope and runs during ordinary returns and exceptions.

## Hopa self-compilation

`hopa` is separate from `bugemoan`. `__hopa__` marks a function that belongs to
a compiler's bootstrap path. The marker is retained in the C+ AST and lowered
away for the native C++ backend.

```c++
__hopa__ int compile_stage(int parser, int *binary) {
    return binary ? parser : 0;
}
```

Use the `hopa` compiler command on a C+ compiler translation unit:

```powershell
cspc hopa selfhost/cspc.csp -o cspc-hopa-stage1.exe
```

The host compiler produces stage 1. Stage 1 then compiles the same source to
`cspc-hopa-stage1-stage2.exe`. The command succeeds only when both stages build.
This is staged self-compilation; placing `__hopa__` on an ordinary function
does not magically turn an application into a compiler.

## Compiler intermediate representations

The compiler pipeline has three inspectable backend levels:

```text
C+ source -> tokens -> AST -> semantic analysis -> THIR -> HIR -> MIR -> LLVM/native
```

- THIR assigns static types to inferred expressions and local declarations.
- HIR retains structured functions, declarations, expressions, records, and
  control flow after semantic validation.
- MIR lowers functions into explicit basic blocks, temporary values, branches,
  jumps, returns, throws, and inline-assembly operations.
- MIR verification runs during every complete frontend compilation and rejects
  unterminated or structurally invalid basic blocks as an internal error.
- `cspc check` returns immediately after verified MIR for fully parsed C+
  programs; backend syntax checking remains only as a compatibility path for
  C++ constructs that the C+ frontend does not yet model.
- Executable builds write a small `.cspmeta` fingerprint beside the output.
  An unchanged repeat skips backend compilation after checking source and
  header timestamps. `--rebuild` bypasses incremental reuse.
- Production LLVM emission currently uses the optimized Clang/LLVM adapter
  after C+ lowering. Direct MIR-to-LLVM instruction selection is not yet used
  for the complete C++-compatible language surface.

```powershell
cspc hir hello.csp                 # hello.hir
cspc mir hello.csp                 # hello.mir
cspc llvm hello.csp                # hello.ll
cspc build hello.csp --emit-hir build/hello.hir --emit-mir build/hello.mir
```

## Performance, ownership, layout, and FFI

C+ templates use compile-time monomorphization through the native backend. A
generic receives a separate optimized instantiation for each concrete type; no
runtime type-erasure object or garbage collector is inserted.

```c++
template <class T>
CSP_FORCE_INLINE T twice(T value) { return value + value; }

auto integer = twice<int>(21);
auto floating = twice<double>(1.5);
```

Records and local values are value types by default. Their storage is automatic
unless the program explicitly requests an owner. Destructors run at lexical
scope exit through RAII. `cp::owner<T>` provides unique heap ownership without
garbage collection.

```c++
#include <abi.h>

struct CSP_ALIGN(64) CacheLine { int values[16]; };
struct CSP_PACKED WireHeader { unsigned short kind; unsigned size; };

static_assert(cp::abi::c_compatible_layout<WireHeader>);
CSP_EXTERN_C int native_api(int value);
```

`CSP_EXTERN_C` selects the platform C calling convention and unmangled symbol
name. `CSP_ALIGN`, `CSP_PACKED`, `cp::abi::size<T>`, and
`cp::abi::alignment<T>` expose data-layout control. Packed fields can be
misaligned, so packed records should be reserved for wire and device layouts.

`auto` remains statically typed. THIR records inferred local types before HIR
lowering; inference does not create a dynamic value. Inspect it with:

```powershell
cspc thir program.csp
```

Constant integer expressions and pure integer `constexpr`/`consteval`
functions in `static_assert` are executed by the frontend CTFE evaluator. It
supports parameter binding and a recursion limit. The native evaluator handles
the broader C++-compatible constant-expression subset.

Exhaustive enum matching uses `match`. A missing enumerator is a compile error:

```c++
enum class State { idle, running, stopped };

int code(State state) {
    match (state) {
    case State::idle: return 0;
    case State::running: return 1;
    case State::stopped: return 2;
    }
}
```

`match` is checked before lowering and becomes a native `switch`; it has no
runtime dispatcher. The `vectorized` profile enables `-O3`, loop vectorization,
and loop unrolling. `--lto`, `--thin-lto`, `--vectorize`, and
`--no-vectorize` expose backend pipeline control. Thin LTO requires a backend
that supports `-flto=thin`, normally Clang.

## Safe bindings and optional values

Bindings are immutable unless `mut` is written explicitly:

```c++
let answer = 42;       // lowered to const auto
let mut total = 0;     // lowered to auto
total += answer;
```

An optional type uses `T?`. Presence must be checked and the value accessed
explicitly; pointer-style dereference of an optional is rejected by semantic
analysis.

```c++
int? port = 8080;
if (port.has_value()) {
    connect(port.value());
}
```

The headers are installed under `.csx/include`. These APIs target native C++
backend builds. The C and Rust transpilers do not currently translate C++
standard-library container types.

## Performance profile and optimizer contracts

Use `performance` for a maximum-speed native build that still preserves normal
integer and floating-point semantics:

```powershell
cspc performance app.csp -o app.exe
```

This enables level-3 optimization, native CPU instructions, link-time
optimization, dead-section elimination, and release assertions. It does not
silently enable `--fast-math`, because fast math changes NaN, infinity, signed
zero, and rounding behavior. Add that option only when the program permits it.

Hot loops can communicate facts that the compiler cannot prove on its own:

```c++
#include <cp/hardware.h>

float sum(const float *CP_RESTRICT input, unsigned count) {
    const float *values = cp::hardware::assume_aligned(input, 32);
    float result = 0.0f;
    for (unsigned i = 0; i < count; ++i) result += values[i];
    return result;
}
```

`CP_RESTRICT` promises that aliased pointers do not overlap,
`assume_aligned` exposes alignment for SIMD vectorization, `CP_LIKELY` and
`CP_UNLIKELY` annotate branches, and the prefetch macros provide cache hints.
Breaking a restrict, alignment, or `CP_ASSUME` promise is undefined behavior;
these tools are for measured hot paths, not ordinary application code.

## cspWindow desktop GUI

Windows applications include `<cspWindow>` and use the `csp::ui` namespace.
The framework provides an event loop, DPI-aware resizable windows, UTF-8 text,
labels, buttons, text boxes, check boxes, progress bars, dialogs, keyboard
events, close events, responsive bounds, and customizable colors.

```c++
#include <cspWindow>
using namespace csp::ui;

int main(void) {
    application app;
    window view({.title = "My C+ App", .width = 900, .height = 600});
    widget greeting = view.label("Hello from C+", {24, 24, 300, 40});
    view.button("Continue", {24, 88, 140, 38}, [&] {
        greeting.set_text("Button clicked");
    });
    view.show();
    return app.run();
}
```

Build with the Windows system libraries:

```powershell
cspc build main.csp -o app.exe
```

The compiler recognizes the GUI header and automatically links the required
Windows system libraries.

## Low-level data and callable primitives

`<lowlevel.h>` provides zero-overhead bit flags, typed function pointers,
compile-time lookup tables, bounded character arrays, string literals, tagged
unions, binary-size number literals, and C-compatible variadic access.

```c++
#include <lowlevel.h>
#include <cpstream>

#define DOUBLE(value) ((value) * 2)
enum class permission : unsigned { read = 1, write = 2, execute = 4 };

struct packet {
    unsigned kind : 3;       // native bitfield
    unsigned urgent : 1;
};

int add(int left, int right) { return left + right; }

int main(void) {
    using namespace cp::literals;
    cp::bit_flags<permission> flags(permission::read);
    flags.set(permission::write);
    cp::function_pointer<int(int, int)> callback = &add;
    constexpr cp::lookup_table<int, 4> square{0, 1, 4, 9};
    constexpr cp::string_literal title("C+");
    cp::char_array<32> text(title.view());
    cp::tagged_union<int, const char *> value =
        cp::tagged_union<int, const char *>::first(21);
    auto memory = 4_KiB;
    printc("%s %d %llu\n", text.c_str(), DOUBLE(value.first()),
           static_cast<unsigned long long>(memory));
    return callback(2, 3) == 5 && square[3] == 9 ? 0 : 1;
}
```

Traditional variadic functions use `CSP_VA_LIST`, `CSP_VA_START`,
`CSP_VA_ARG`, and `CSP_VA_END`. Ordinary formatted output should prefer
`printc`, whose typed template arguments avoid manual `va_list` handling.
Number tokens support decimal, hexadecimal, binary, octal, separators,
floating-point exponents, and suffixes. String, raw-string, and character
literals are recognized by the lexer rather than treated as unparsed text.

## CP ASM (`.cpsm`)

CP ASM is the low-level assembly language shipped with C+. It uses `.cpsm`
source files and is handled by a dedicated lexer, parser, statement AST, and
validated GNU-assembler emitter. It is not treated as C+ or copied blindly to
the platform assembler.

```asm
; hello.cpsm
.target x86_64

.proc main
    xor eax, eax
    ret
.end
```

Compile, check, emit native assembly, or create an object file:

```powershell
cspc check examples/hello.cpsm
cspc asm examples/hello.cpsm -o hello.s
cspc object examples/hello.cpsm -o hello.o
cspc build examples/hello.cpsm -o hello.exe
```

CP ASM accepts `;` and `//` comments, labels such as `loop:`, bracketed memory
operands, quoted strings, decimal/hexadecimal/binary integers, and native
instruction operands. The core directives are:

| Directive | Purpose |
| --- | --- |
| `.target x86_64` / `.target aarch64` | Select the instruction architecture |
| `.proc name` / `.end` | Define a global procedure |
| `.section text|data|rodata` | Select a section |
| `.global name` / `.extern name` | Declare symbol visibility |
| `.const name, value` | Define an assembler constant |
| `.align n` | Request alignment |
| `.byte`, `.word`, `.dword`, `.qword` | Emit fixed-width data |
| `.ascii`, `.asciz` | Emit string data |

x86-64 CP ASM uses Intel operand order and register spelling. AArch64 uses its
native register and operand conventions. Cross-architecture builds require a
backend assembler that supports the requested `--target`.

The canonical C+ program entry point remains C-style:

```c++
int main(void) {
    return 0;
}
```
