# LINK386 R4C — Source-only Microsoft C/386 runtime object materialization

**Status:** Two object contents produced directly from `hi.obj + LIBC.LIB + OS2386.LIB`. Not yet a standalone runnable LE linker.

R4B is frozen/live-proven for reference-assisted linking. R4C is a separate source tree. No Microsoft `hi.exe` is required for the R4C materializers; the reference is used only by verification tests to quantify remaining differences.

```sh
make link386r4c link386r4c_images link386r4c_relocate
make verify-r4c
mkdir -p build/r4c
./link386r4c_images build/r4c fixtures/hi.obj fixtures/LIBC.LIB fixtures/OS2386.LIB
```

The resulting `build/r4c/object1-unrelocated.bin` and `object2-unrelocated.bin` have the exact 6174-byte code and 10256-byte data/stack lengths of Microsoft's hello LE objects. `objectN-mask.bin` labels ordinary bytes, relocation operands, and unsourced/zero-filled bytes. `objectN-candidate-relocated.bin` is **diagnostic** and is not executable.

GCC and Clang compile the C source with `-O2 -std=c89 -pedantic -Wall -Wextra -Werror`. Test and detailed handoff: `HANDOFF-R4C.md`.
