window.CSP_BENCHMARK_DATA = {
  "generated_at": "2026-09-05T16:11:16Z",
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
      "compile_ms": 885.356,
      "median_ms": 93.885,
      "min_ms": 90.261,
      "max_ms": 104.805,
      "binary_bytes": 254641,
      "checksum": "15305858211001835799",
      "samples_ms": [
        91.729,
        90.261,
        104.805,
        98.569,
        91.437,
        93.885,
        94.347
      ],
      "command": [
        "C:\\Users\\DeLL\\Desktop\\C+\\cspc.exe",
        "C:\\Users\\DeLL\\Desktop\\C+\\benchmarks\\workload.csp",
        "--no-runtime",
        "--compiler",
        "C:\\msys64\\mingw64\\bin\\g++.exe",
        "-O3",
        "-o",
        "C:\\Users\\DeLL\\Desktop\\C+\\benchmarks\\build\\workload-csp.exe"
      ]
    },
    {
      "language": "C",
      "status": "ok",
      "compiler": "gcc.exe (Rev6, Built by MSYS2 project) 16.1.0",
      "compile_ms": 773.332,
      "median_ms": 106.27,
      "min_ms": 91.769,
      "max_ms": 109.37,
      "binary_bytes": 254491,
      "checksum": "15305858211001835799",
      "samples_ms": [
        109.37,
        109.231,
        106.27,
        100.091,
        92.061,
        91.769,
        106.388
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
      "compile_ms": 348.757,
      "median_ms": 94.54,
      "min_ms": 91.86,
      "max_ms": 104.71,
      "binary_bytes": 254491,
      "checksum": "15305858211001835799",
      "samples_ms": [
        94.639,
        94.944,
        104.71,
        94.54,
        94.502,
        93.418,
        91.86
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
      "compile_ms": 26439.209,
      "median_ms": 111.589,
      "min_ms": 108.025,
      "max_ms": 138.706,
      "binary_bytes": 4906185,
      "checksum": "15305858211001835799",
      "samples_ms": [
        138.706,
        112.364,
        111.589,
        108.192,
        111.869,
        110.755,
        108.025
      ],
      "command": [
        "C:\\Users\\DeLL\\.cargo\\bin\\rustc.exe",
        "-C",
        "opt-level=3",
        "C:\\Users\\DeLL\\Desktop\\C+\\benchmarks\\workload.rs",
        "-o",
        "C:\\Users\\DeLL\\Desktop\\C+\\benchmarks\\build\\workload-rust.exe"
      ]
    }
  ]
}
;
