# CSP code-generation test suite

This suite contains 5,094 deterministic C+ regression programs across 85 case
folders. It preserves all 93 supplied architecture/assembly names, then adds a
parameterized matrix spanning integer, array, structure, floating-point,
string, pointer, and recursion workloads across host and cross-target metadata.
It does not copy Rust source: every case is a `.csp` program.

```powershell
csp-codegen --validate-only
csp-codegen
csp-codegen --compile-host
csp-codegen --shard 1/85 --jobs 8
```

The default run passes all sources through the real CSP lexer, parser, and
semantic checker. `--compile-host` additionally emits optimized assembly for
host cases. Cross-target metadata is preserved in `manifest.json`; emitting a
cross-target case requires a Clang build that supports that target.

The suite is sharded for CI so 85 workers can each check roughly 60 cases.
File count is not treated as feature count: a case is useful only when it maps
to a compiler behavior and passes structural validation.
