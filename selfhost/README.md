# C+ bootstrap

The checked-in native compiler is stage 1. It compiles `cspc.csp`, which is a C+ translation unit containing the compiler implementation, to produce stage 2. Stage 2 must then compile and run a normal C+ program.

```powershell
.\cspc.exe selfhost/cspc.csp --no-runtime -I . -O2 -o cspc-stage2.exe
.\cspc-stage2.exe hello.csp -o hello-stage2.exe
.\hello-stage2.exe
```

The implementation currently uses the C++-compatible subset of C+. This is intentional bootstrap engineering: new C+-only constructs can replace the compatible subset once the stage-2 compiler implements them itself.
