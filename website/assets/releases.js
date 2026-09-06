window.CSP_RELEASE_DATA = {
  current: "0.2.0",
  artifacts: [
    {name:"CSP compiler",version:"0.2.0",platform:"windows",platformLabel:"Windows x86-64 · standalone",bytes:3738049,sha256:"65e0ea1f259bfe8c8b11bb17fc9dd46c552a584f4270031b31d267582abecb4f",url:"downloads/cspc-0.2.0-windows-x86_64.exe"},
    {name:"CSP source release",version:"0.2.0",platform:"source",platformLabel:"Portable source",bytes:357434,sha256:"43d48e22f57c77cc4fbe8aca124e376b0718c203f57bcc58cb7f9b96f253b440",url:"downloads/csp-0.2.0-source.tar.gz"},
    {name:"CSP standard libraries",version:"0.2.0",platform:"source",platformLabel:"Header archive",bytes:16798,sha256:"49d912a4ea55c928fca45e90a7a6d56180fe06e7205114d740179dd7093ae6e8",url:"downloads/csp-libraries-0.2.0.zip"},
    {name:"Arch Linux package bundle",version:"0.2.0",platform:"arch",platformLabel:"PKGBUILD + source",bytes:284123,sha256:"f7c8940cda7041250105276e08e5d73fbe583dc645500a0e5052e4ddfd7a167e",url:"downloads/csp-0.2.0-arch-package.zip"}
  ],
  packages: [
    {name:"csp",version:"0.2.0-1",arch:"x86_64",repository:"Core",description:"Configuration Senior Programming compiler and complete standard library",updated:"2026-09-04",license:"MIT",installedSize:"3.7 MiB",maintainer:"CSP Foundation",dependencies:["gcc-libs"],provides:"cspc=0.2.0",artifacts:["CSP compiler","CSP source release","Arch Linux package bundle"],files:["usr/bin/cspc","usr/include/cpstream","usr/include/vectors.h","usr/include/cp/*.h","usr/share/doc/csp/*"]},
    {name:"csp-compiler",version:"0.2.0-1",arch:"x86_64",repository:"Core",description:"Self-hosting CSP compiler with C, Rust, LLVM, assembly, C++ and CUDA targets",updated:"2026-09-04",license:"MIT",installedSize:"3.6 MiB",maintainer:"CSP Foundation",dependencies:["gcc-libs"],provides:"cspc",artifacts:["CSP compiler","CSP source release"],files:["usr/bin/cspc","usr/share/doc/csp/README.md","usr/share/doc/csp/syntax.md"]},
    {name:"cpstream",version:"0.2.0-1",arch:"any",repository:"Extra",description:"Formatted output and stream primitives for CSP",updated:"2026-09-04",license:"MIT",installedSize:"4 KiB",maintainer:"CSP Foundation",dependencies:[],provides:"cpstream",artifacts:["CSP standard libraries"],files:["usr/include/cpstream"]},
    {name:"cp-vectors",version:"0.2.0-1",arch:"any",repository:"Extra",description:"Fixed, dynamic, PMR, and non-owning vector containers",updated:"2026-09-04",license:"MIT",installedSize:"12 KiB",maintainer:"CSP Foundation",dependencies:[],provides:"vectors.h",artifacts:["CSP standard libraries"],files:["usr/include/vectors.h","usr/include/cp/vector.h"]},
    {name:"cp-cuda",version:"0.2.0-1",arch:"x86_64",repository:"Extra",description:"Checked CUDA runtime operations and RAII device buffers",updated:"2026-09-04",license:"MIT",installedSize:"8 KiB",maintainer:"CSP Foundation",dependencies:["cuda (optional at compile time)"],provides:"cp-cuda",artifacts:["CSP standard libraries"],files:["usr/include/cp/cuda.h"]},
    {name:"cp-core",version:"0.3.0-1",arch:"any",repository:"Core",description:"CSP types, memory, atomics, math, results, scopes, algorithms and hardware utilities",updated:"2026-09-04",license:"MIT",installedSize:"44 KiB",maintainer:"CSP Foundation",dependencies:[],provides:"cp-stdlib",artifacts:["CSP standard libraries"],files:["usr/include/cp/algorithm.h","usr/include/cp/atomic.h","usr/include/cp/hardware.h","usr/include/cp/math.h","usr/include/cp/memory.h","usr/include/cp/result.h","usr/include/cp/scope.h","usr/include/cp/types.h","usr/include/cp/vector.h"]},
    {name:"cp-platform",version:"0.3.0-1",arch:"any",repository:"Core",description:"Kernel, Linux and GNU platform primitives",updated:"2026-09-04",license:"MIT",installedSize:"8 KiB",maintainer:"CSP Foundation",dependencies:["cp-core"],provides:"cp-platform",artifacts:["CSP standard libraries"],files:["usr/include/cp/kernel.h","usr/include/cp/linux.h","usr/include/cp/gnu.h"]},
    {name:"cp-network",version:"0.3.0-1",arch:"any",repository:"Extra",description:"Portable sockets, HTTP request helpers and site URL parsing",updated:"2026-09-04",license:"MIT",installedSize:"12 KiB",maintainer:"CSP Foundation",dependencies:["cp-platform"],provides:"cp-network",artifacts:["CSP standard libraries"],files:["usr/include/cp/socket.h","usr/include/cp/http.h","usr/include/cp/site.h"]},
    {name:"cp-system",version:"0.4.0-1",arch:"any",repository:"Extra",description:"Handwritten filesystem, networking, process, time, and serialization APIs",updated:"2026-09-04",license:"MIT",installedSize:"18 KiB",maintainer:"CSP Foundation",dependencies:["cp-core"],provides:"cp-system",artifacts:["CSP standard libraries"],files:["usr/include/cp/filesystem.h","usr/include/cp/networking.h","usr/include/cp/process.h","usr/include/cp/serialization.h","usr/include/cp/time.h"]}
    ,{name:"cp-containers",version:"0.5.0-1",arch:"any",repository:"Extra",description:"Ordered and hashed maps, sets, lists, and fixed arrays",updated:"2026-09-04",license:"MIT",installedSize:"16 KiB",maintainer:"CSP Foundation",dependencies:["cp-core"],provides:"cp-containers",artifacts:["CSP standard libraries"],files:["usr/include/cp/array.h","usr/include/cp/list.h","usr/include/cp/map.h","usr/include/cp/set.h"]}
    ,{name:"csp-stdlib",version:"0.5.0-1",arch:"any",repository:"Core",description:"Complete aggregate CSP standard library",updated:"2026-09-04",license:"MIT",installedSize:"2 KiB",maintainer:"CSP Foundation",dependencies:["cp-core","cp-containers","cp-platform","cp-network","cp-system"],provides:"cp-stdlib",artifacts:["CSP standard libraries"],files:["usr/include/cp/stdlib.h"]}
    ,{name:"csx-studio",version:"0.5.0-1",arch:"any",repository:"Extra",description:"Project models, build targets and command generation for CSP tooling",updated:"2026-09-04",license:"MIT",installedSize:"8 KiB",maintainer:"CSP Foundation",dependencies:["csp-stdlib"],provides:"csxStudio.h",artifacts:["CSP standard libraries"],files:["usr/include/csxStudio.h"]}
  ],
  libraries: [
    {name:"<cpstream>",package:"cpstream",description:"Formatted printing with printc, print, and println."},
    {name:"<vectors.h>",package:"cp-vectors",description:"Hardware-friendly fixed-size arithmetic vectors."},
    {name:"<cp/vector.h>",package:"cp-vectors",description:"Dynamic, PMR, and non-owning vector containers."},
    {name:"<cp/types.h>",package:"cp-core",description:"Exact-width integers, sizes, and Unicode character types."},
    {name:"<cp/memory.h>",package:"cp-core",description:"Ownership aliases, spans, copying, and zeroing."},
    {name:"<cp/result.h>",package:"cp-core",description:"Explicit value-or-error results with mapping."},
    {name:"<cp/scope.h>",package:"cp-core",description:"Deterministic scope-exit cleanup actions."},
    {name:"<cp/algorithm.h>",package:"cp-core",description:"Range algorithms and numeric helpers."},
    {name:"<cp/math.h>",package:"cp-core",description:"Constants, interpolation, clamping, and bit operations."},
    {name:"<cp/atomic.h>",package:"cp-core",description:"Atomics, fences, and named memory ordering."},
    {name:"<cp/cuda.h>",package:"cp-cuda",description:"Checked CUDA calls and RAII device memory."},
    {name:"<cp/hardware.h>",package:"cp-core",description:"Inlining, prediction, prefetching, and device annotations."},
    {name:"<cp/kernel.h>",package:"cp-platform",description:"Portable launch shapes and kernel thread indexing."},
    {name:"<cp/linux.h>",package:"cp-platform",description:"Guarded Linux process and system capability helpers."},
    {name:"<cp/gnu.h>",package:"cp-platform",description:"GNU and Clang optimization attributes."},
    {name:"<cp/socket.h>",package:"cp-network",description:"Portable socket endpoints and handle cleanup."},
    {name:"<cp/http.h>",package:"cp-network",description:"HTTP/1.1 request and response primitives."},
    {name:"<cp/site.h>",package:"cp-network",description:"URL parsing and endpoint selection."}
    ,{name:"<cp/array.h>",package:"cp-containers",description:"Fixed-size arrays, byte arrays, and construction helpers."}
    ,{name:"<cp/map.h>",package:"cp-containers",description:"Ordered, hashed, multi-map, PMR, and safe lookup APIs."}
    ,{name:"<cp/set.h>",package:"cp-containers",description:"Ordered and hashed sets with union and intersection."}
    ,{name:"<cp/list.h>",package:"cp-containers",description:"Linked and forward lists with insertion helpers."}
    ,{name:"<cp/new.h>",package:"cp-core",description:"Placement construction and monotonic arena allocation."}
    ,{name:"<csxStudio.h>",package:"csx-studio",description:"IDE projects, build targets, diagnostics, and compiler commands."}
  ],
  releases: [
    {version:"0.2.0",date:"2026-09-04",channel:"current",summary:"Self-hosting bootstrap, native/C/Rust/LLVM/assembly targets, CUDA parsing, semantic analysis, and standard-library foundation."},
    {version:"0.1.0",date:"2026-09-04",channel:"bootstrap",summary:"Initial portable C+ compiler and native executable generation."}
  ]
};
