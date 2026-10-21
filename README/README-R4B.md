# R4B experimental forced EXE

**Status: FIRST EXE EMITTED (REFERENCE-ASSISTED); NOT A STANDALONE LINKER.**

We discovered and corrected the prior page-map misconception: Microsoft's `hi.exe` maps logical LE pages 1/2/3 to disk physical pages 2/3/1. This places the original application `_main` in object 1 at 0x10010, C runtime startup at 0x10020, and its data at object 2 offset 0x44. The heap code is in LE object 1, not object 2.

R4B adds `link386r4b`, a strict-C89 tool that reads the original OMF input and both Microsoft libraries, selects 39 modules, validates the 171 inherited loader-fixup sites, overlays the actual application contribution bytes and applies the two real application 32-bit relocations against the verified existing runtime scaffold. The unchanged output is identical to Microsoft's hi.exe. A checksum-correct alternative OMF source changes the generated executable's string from `hi!` to `yo!` while retaining all loader fixups.

```sh
make link386r4b verify-r4b
./link386r4b -r fixtures/hi-reference.exe -o build/hi-r4b.exe fixtures/hi.obj fixtures/LIBC.LIB fixtures/OS2386.LIB
./link386r4b --allow-data-edit -r fixtures/hi-reference.exe -o build/hi-yo-r4b.exe fixtures/hi-yo.obj fixtures/LIBC.LIB fixtures/OS2386.LIB
```

**Limitation:** The original Microsoft exe is REQUIRED (`-r`) as a linked runtime and fixup scaffold. This is a proof that the application OMF relocation pathway works, not a proof that the complete Microsoft C/386 runtime can be freshly linked by our code. No runtime execution occurred in the model's Linux environment. Full details and correction of previous findings are in `HANDOFF-R4B.md`.
