;; Package-name fingerprint used by the CSP catalog search page.
(module
  (memory (export "memory") 1)
  (func $fnv1a (export "fnv1a") (param $ptr i32) (param $len i32) (result i32)
    (local $i i32)
    (local $hash i32)
    (local.set $hash (i32.const 2166136261))
    (loop $words
      (if (i32.lt_u (local.get $i) (local.get $len))
        (then
          (local.set $hash
            (i32.mul
              (i32.xor
                (local.get $hash)
                (i32.load8_u (i32.add (local.get $ptr) (local.get $i))))
              (i32.const 16777619)))
          (local.set $i (i32.add (local.get $i) (i32.const 1)))
          (br $words))))
    (local.get $hash))
)
