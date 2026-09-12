window.CSP_BENCHMARK_DATA = {
  "generated_at": "2026-09-08T09:15:31Z",
  "machine": {
    "architecture": "amd64",
    "cpu": "Intel64 Family 6 Model 165 Stepping 2, GenuineIntel",
    "os": "windows",
    "runner": "Go go1.27.0"
  },
  "method": {
    "optimization": "release / level 3",
    "rounds": 25000000,
    "runs": 7,
    "timer": "wall clock including process startup",
    "warmup": 2,
    "workload": "xorshift64* integer mixing"
  },
  "results": [
    {
      "language": "C+",
      "status": "ok",
      "compiler": "C+ 0.2.0 via g++.exe (Rev6, Built by MSYS2 project) 16.1.0",
      "compile_ms": 9240.521,
      "median_ms": 165.401,
      "min_ms": 158.792,
      "max_ms": 233.276,
      "binary_bytes": 254641,
      "checksum": "15305858211001835799",
      "samples_ms": [
        162.367,
        161.886,
        158.792,
        188.41,
        165.401,
        233.276,
        204.168
      ],
      "command": [
        "C:\\Users\\DeLL\\Desktop\\C+\\build\\cspc.exe",
        "C:\\Users\\DeLL\\Desktop\\C+\\benchmarks\\workload.csp",
        "--no-runtime",
        "--compiler",
        "C:\\msys64\\mingw64\\bin\\g++.exe",
        "-O3",
        "--rebuild",
        "-o",
        "C:\\Users\\DeLL\\Desktop\\C+\\benchmarks\\build\\workload-csp.exe"
      ]
    },
    {
      "language": "C",
      "status": "ok",
      "compiler": "gcc.exe (Rev6, Built by MSYS2 project) 16.1.0",
      "compile_ms": 4043.206,
      "median_ms": 115.615,
      "min_ms": 102.672,
      "max_ms": 118.711,
      "binary_bytes": 254491,
      "checksum": "15305858211001835799",
      "samples_ms": [
        118.018,
        115.615,
        118.711,
        102.672,
        116.965,
        107.475,
        105.305
      ],
      "command": [
        "C:\\msys64\\mingw64\\bin\\gcc.exe",
        "-std=c17",
        "-O3",
        "C:\\Users\\DeLL\\Desktop\\C+\\benchmarks\\workload.c",
        "-o",
        "C:\\Users\\DeLL\\Desktop\\C+\\benchmarks\\build\\workload-c.exe"
      ]
    },
    {
      "language": "C++",
      "status": "ok",
      "compiler": "g++.exe (Rev6, Built by MSYS2 project) 16.1.0",
      "compile_ms": 645.326,
      "median_ms": 165.066,
      "min_ms": 148.375,
      "max_ms": 203.265,
      "binary_bytes": 254491,
      "checksum": "15305858211001835799",
      "samples_ms": [
        189.995,
        203.265,
        191.657,
        163.188,
        148.375,
        154.46,
        165.066
      ],
      "command": [
        "C:\\msys64\\mingw64\\bin\\g++.exe",
        "-std=c++20",
        "-O3",
        "C:\\Users\\DeLL\\Desktop\\C+\\benchmarks\\workload.cpp",
        "-o",
        "C:\\Users\\DeLL\\Desktop\\C+\\benchmarks\\build\\workload-cpp.exe"
      ]
    },
    {
      "language": "Rust",
      "status": "ok",
      "compiler": "rustc 1.97.1 (8bab26f4f 2026-07-14)",
      "compile_ms": 37779.444,
      "median_ms": 122.665,
      "min_ms": 112.264,
      "max_ms": 136.003,
      "binary_bytes": 4906185,
      "checksum": "15305858211001835799",
      "samples_ms": [
        122.665,
        120.353,
        128.473,
        136.003,
        130.663,
        112.264,
        115.546
      ],
      "command": [
        "C:\\Users\\DeLL\\.cargo\\bin\\rustc.exe",
        "-C",
        "opt-level=3",
        "C:\\Users\\DeLL\\Desktop\\C+\\benchmarks\\workload.rs",
        "-o",
        "C:\\Users\\DeLL\\Desktop\\C+\\benchmarks\\build\\workload-rust.exe"
      ]
    },
    {
      "language": "ASM x86-64",
      "status": "ok",
      "compiler": "gcc.exe (Rev6, Built by MSYS2 project) 16.1.0",
      "compile_ms": 299.66,
      "median_ms": 107.278,
      "min_ms": 94.367,
      "max_ms": 121.488,
      "binary_bytes": 102460,
      "checksum": "15305858211001835799",
      "samples_ms": [
        120.963,
        107.278,
        121.488,
        105.663,
        109.725,
        101.866,
        94.367
      ],
      "command": [
        "C:\\msys64\\mingw64\\bin\\gcc.exe",
        "C:\\Users\\DeLL\\Desktop\\C+\\benchmarks\\workload_windows_x86_64.s",
        "-o",
        "C:\\Users\\DeLL\\Desktop\\C+\\benchmarks\\build\\workload-asm.exe"
      ]
    }
  ]
}
;
