# c+ programming

print

#include <cpstream>

int main(){
    printc("hello world");
    return 0;

}

variable

#include <cpstream>

int main(){
    int num = 2;
    printc("hi %d"num);
    return 0;
    
}

arrays

#include <cpstream>

int main(){
    int num[3] = 1, 3, 7;
    printc("hi %d"num);
    return 0;
    
}

data types

int, void, signed, unsigned, double, float, u16, u8, u32, u64, u128, goto, char, unichar, size_t


float variable example 

#include <cpstream>

int main(){
    float num = 2.01;
    printc("hi %d"num);
    return 0;
    
}

for loops

#include <cpstream>

int main(){
    for(int i < 50, i++;){
        printc("hello world");
    }
    return 0;
}
while loops

#include <cpstream>

int main(){
    bool walk = false;
    while(bool walk == false){
        printc("you can't walk because the bool value is false");
    }
    return 0;
}

goto loops

#include <cpstream>

int main(){
    Goto Label:
    printc("hello world");
    Goto Stop[label]
return 0;
}

vectors

#include <cpstream>

int main(){
    int vec = {1.0f, 1.5f, 2.0f};
    printc("vector is %d");
    return 0;
}

pointers

#include <cpstream>

int main(){
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
  int main(){
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

int main() {
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

int main() {
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

int main() {
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

int main() {
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

int main() {
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

int main() {
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

int main() {
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

int main() {
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

int main() {
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

The headers are installed under `.csx/include`. These APIs target native C++
backend builds. The C and Rust transpilers do not currently translate C++
standard-library container types.
