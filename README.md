# C+

C+ is a systems programming language with C/CUDA-inspired syntax and the `.csp` file extension. Its native C++ compiler supports executable, C, Rust, LLVM IR, assembly, and CUDA-oriented workflows, with a bundled standard library and the compiled Go-powered CSX package manager. The package manager, website server, regression runner, and benchmark runner are native Go executables; Python is not required.

The frontend pipeline includes semantic analysis, THIR type inference,
structured HIR, verified control-flow MIR, constant evaluation, and exhaustive
enum matching. Native builds use value semantics, RAII, monomorphized templates,
C-compatible ABI controls, and LLVM/GCC optimization pipelines without a
garbage collector.

## Install commands on PATH

On Windows, build the Release configuration and run the user installer once:

```powershell
powershell -ExecutionPolicy Bypass -File tools/install-user.ps1
```

Open a new terminal afterward. `cspc`, `csx`, `cspweb`, `csp-codegen`, and
`csp-bench` will then work from every directory without a `.\` prefix. The
default installation root is `%LOCALAPPDATA%\CSP`; pass `-Prefix` to select a
different per-user location.

## Quick start

Build the compiler once:

```powershell
g++ -std=c++20 -O2 cspc.cpp src/lexer.cpp src/parser.cpp -o cspc.exe
```

Compile and run a C+ program:

```powershell
.\cspc.exe hello.csp -o hello.exe
.\hello.exe
```

Build with a chosen output name:

```powershell
.\cspc.exe program.csp -o program.exe
```

Use `--emit-cpp generated.cpp` to inspect the generated C++, `--compiler clang++` to choose a backend, or `--run` to run the native executable after building it.

`cspc check` stops after the C+ frontend has completed semantic analysis and
verified THIR, HIR, and MIR. It does not start the C++ backend when the parser
fully recognizes the source, keeping edit-time checks fast.

Executable builds are incremental. Repeating the same command reuses the
output only when the source, options, and relevant CSP headers are unchanged.
Use `--rebuild` for a guaranteed cold backend build; the bundled benchmark uses
this option so its compile-time result is not a cache hit.

Generate optimized LLVM IR or native assembly:

```powershell
.\cspc.exe program.csp --emit-llvm -O3 -o program.ll
.\cspc.exe program.csp -S --native -O3 -o program.s
```

Build a CPU-tuned executable with level-3 optimization, LTO, and dead-code
elimination while preserving normal floating-point semantics:

```powershell
cspc performance program.csp -o program.exe
```

Add `--fast-math` only when the application permits relaxed NaN, infinity,
signed-zero, and rounding behavior.

Transpile the C-compatible C+ subset to portable C or the currently supported
AST subset to Rust:

```powershell
.\cspc.exe program.csp --emit-c -o program.c
.\cspc.exe program.csp --emit-rust -o program.rs
```

The C target rejects classes, templates, namespaces, exceptions, and CUDA
kernels rather than silently changing their meaning. The Rust target currently
supports functions, declarations, expressions, calls, `printc`, conditions,
loops, returns, indexing, and initializer lists. Unsupported nodes produce a
source-located diagnostic.

Cross-compile freestanding code without a hosted runtime:

```powershell
.\cspc.exe firmware.csp --compiler clang++ --freestanding -S `
  --target aarch64-none-elf --cpu cortex-a72 -O3 -o firmware.s
```

`--target` accepts backend-supported target triples. `--cpu`, `--native`, `--lto`, and `--fast-math` expose architecture tuning and optimization. `--check` performs syntax and semantic checks without code generation. `--ast-json tree.json` exports the AST for tools.

Inspect the real compiler frontend:

```powershell
.\cspc.exe hello.csp --dump-tokens --dump-ast -o hello.exe
```

## Compiler architecture

- `src/token.hpp` defines tokens and exact source locations.
- `src/lexer.cpp` performs character-by-character lexical analysis, including comments, literals, numeric forms, operators, CUDA-style launch brackets, and preprocessor lines.
- `src/ast.hpp` owns the typed abstract syntax tree.
- `src/parser.cpp` is the recursive-descent structural parser for translation units, functions, blocks, control flow, labels, returns, and expressions.
- `cspc.cpp` drives diagnostics and native code generation.

The AST frontend runs on every compilation. `--dump-tokens` and `--dump-ast` expose its output for language development.

The semantic pass enforces function-local labels and gotos, loop/switch control
boundaries (including lambda boundaries), unique switch defaults, declarations,
function overload sets, prototype/definition pairing, and duplicate-definition
diagnostics before a native backend is invoked.

## Standard library

The bundled include directory is discovered relative to `cspc.exe`:

- `<cpstream>`: `printc`, `cp::print`, and `cp::println`
- `<vectors.h>`: fixed-size arithmetic vectors such as `cp::float3`
- `<cp/vector.h>`: dynamic, PMR, and non-owning vector types
- `<cp/array.h>`: fixed-size and byte arrays with construction helpers
- `<cp/map.h>`: ordered, hashed, multi-map, PMR, and safe lookup APIs
- `<cp/set.h>`: ordered and hashed sets with union/intersection helpers
- `<cp/list.h>`: linked and forward lists with append/erase helpers
- `<cp/types.h>`: exact-width integer and Unicode types
- `<cp/memory.h>`: unique/shared ownership, spans, alignment, and byte operations
- `<cp/new.h>`: placement construction and monotonic arena allocation
- `<cp/result.h>`: explicit success/error results
- `<cp/scope.h>`: deterministic scope-exit actions
- `<cp/algorithm.h>`: ranges algorithms and numeric helpers
- `<cp/hardware.h>`: inlining, branch prediction, prefetching, and host/device annotations
- `<cp/kernel.h>`: portable launch shapes and thread-index helpers
- `<cp/linux.h>`: Linux capability checks, process IDs, and page sizes
- `<cp/gnu.h>`: GNU/Clang prediction and inlining annotations
- `<cp/socket.h>`: portable socket endpoints and close helpers
- `<cp/http.h>`: HTTP request/response primitives for transport layers
- `<cp/site.h>`: URL parsing and site endpoint helpers
- `<cp/stdlib.h>`: aggregate standard-library header
- `<csxStudio.h>`: IDE project models, targets, diagnostics, and build commands

Short compatibility includes—`<map.h>`, `<set.h>`, `<list.h>`, `<array.h>`,
`<memory.h>`, and `<new.h>`—forward to the corresponding `cp/` modules. The
`cp/` names remain canonical so they do not collide with C++ headers such as
`<map>` and `<memory>`.

## CSX package manager

`csx` is the offline-first package manager for the bundled headers. It reads the
checked-in registry at `packages/csx/index.json`, resolves dependencies, and
installs headers into a prefix without requiring a network connection:

```powershell
csx list
csx search network
csx --prefix .csx install cp-network
csx --prefix .csx install cp-system
csx --prefix .csx install cp-containers csx-studio
csx --prefix .csx installed
csx --prefix .csx verify
```

The installed headers are available under `.csx/include`; pass that directory
to native compilation with `-I .csx/include`. CMake installs the registry and
CLI beside `cspc` for packaged distributions.

`containers_demo.csp` exercises maps, sets, lists, arrays, ownership, arena
allocation, and the CSX Studio project model in one native program.

`cp-system` contains handwritten filesystem, networking, process, time, and
serialization headers. Every module has a focused API and includes the CSP
Foundation MIT notice; no generator is required to build or use the library.

## CUDA backend

Install the NVIDIA CUDA Toolkit so `nvcc` is on `PATH`, then compile GPU programs with:

```powershell
.\cspc.exe kernel.csp --cuda --gpu-arch sm_86 -o kernel.exe
```

C+ recognizes `__global__` kernels and `kernel<<<grid, block>>>(...)` launch expressions in its lexer and AST. CUDA mode emits a `.cu` translation unit and invokes NVIDIA's native CUDA backend. Normal C++ mode uses `g++`. Native include, definition, library, optimization, debug, and compile-only flags are accepted through `-I`, `-D`, `-L`, `-l`, `-O`, `-g`, and `-c`.

## Honest benchmarks

The reproducible suite compares equivalent C+, C, C++, and Rust integer
workloads, verifies their checksums, and records warm-ups, all timing samples,
compiler versions, flags, compile time, and binary size:

```powershell
csp-bench --runs 7 --warmup 2 --rounds 25000000
```

C+ currently lowers to C++, so similar generated-program performance is
expected while C+ compilation includes an additional frontend pass. See
`benchmarks/README.md` for the limitations; the website reports the generated
results without claiming one workload proves a universally fastest language.

## Architecture and assembly regression tests

`tests/codegen` contains 5,094 C+ architecture/code-generation cases across 85
case folders. It preserves all 93 supplied names and expands them with a
deterministic target/workload matrix. The parallel runner checks every source
with the real compiler frontend, while host cases can emit optimized assembly:

```powershell
csp-codegen
csp-codegen --compile-host
csp-codegen --shard 1/85 --jobs 8
```

The original `.rs` names were translated to `.csp`; Rust tests would not test
the CSP compiler. Cross-target cases stay tagged with their target triple and
can be emitted when the selected Clang backend supports that architecture.

## Self-hosting bootstrap

Stage 1 can compile the C+ compiler translation unit into stage 2:

```powershell
.\cspc.exe selfhost/cspc.csp --no-runtime -I . -O2 -o cspc-stage2.exe
.\cspc-stage2.exe hello.csp -o hello-stage2.exe
```

The bootstrap is tested further by having stage 2 emit C, Rust, LLVM IR, and
assembly. See `selfhost/README.md` for the bootstrap model.

## Implemented language features

- C/C++ declarations, expressions, functions, pointers, `if`, `while`, and standard `for` loops
- Built-in `printc`, plus C+ shorthand such as `printc("value: %d"value)`
- C+ loop shorthand `for(int i < 50, i++;)` (starts `i` at zero)
- Array shorthand such as `int nums[3] = 1, 3, 7;`
- Types `u8`, `u16`, `u32`, `u64`, `u128`, and `unichar`
- `#include <cpstream>` as the standard C+ runtime import

GPU syntax (`__global__`, kernel launch brackets, and GPU thread identifiers) is reserved but not implemented by the portable backend. The compiler reports this explicitly.

The examples in [syntax.md](syntax.md) are the language design notes; [hello.csp](hello.csp) is a runnable example.
