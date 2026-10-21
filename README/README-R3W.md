# LINK386 R3W — cross-object `_TEXT` placement evidence

Base: R3V frozen byte-for-byte. The C linker and previous oracle code are unchanged. This iteration adds a read-only, independent and reproducible OMF-to-LE placement regression test.

## Verified

Eight code import sites in the Microsoft `hi.exe` are shifted by `+0x1010` relative to R3N's predictions (R3V). Five imported runtime `_TEXT` contributions are fingerprinted to LE object 1 (`crt0.asm`, `crt0dat.asm`, `nmsghdr.asm`, `lseek.asm`, `write.asm`). **Crucially, two other genuine OMF `_TEXT` contributions have unique byte-window anchors inside LE object 2**: `heapgrow.c` at `0x202A0`, and `heapinit.c` at `0x204E4`. Thus an OMF segment name `_TEXT` does *not* imply placement in the output's first 32-bit code object.

Four confirmed LE object 2 import source sites are `0x20374`/DOSCALLS.299, `0x20456`/305, `0x204AE`/304 and `0x204FF`/299. The first three are located 0xD4/0x1B6/0x20E bytes past the fingerprinted heapgrow `_TEXT` anchor; the last is 0x1B past heapinit's. **Their exact source OMF fixup records have not yet been independently matched to these offsets.** This is a constrained association, not an end-to-end relocation proof. Their actual location follows from the LE fixup stream, independently of the fingerprints.

A Microsoft C/386 executable can use familiar small-model segment names and groups while containing flat 32-bit code; segment nomenclature and the OMF use16/use32 attribute must not be conflated. The precise reason for the `+0x1010` code shift is still unknown. A 4 KiB reserved prefix followed by 16-byte alignment is a plausible *hypothesis*, not yet a demonstrated fact.

## Reproduce

`make verify-r3w` runs the new ownership assertions plus R3V and R3S regressions. Output: `r3w-ownership.txt`. No code patching, no relocation emission, no new LE.

## Next

Recover the original `_TEXT` segment classes, GROUP members and OMF fixup sites for both heap modules; inspect why the linker partitioned code across two LE objects; then update actual layout code under a separately tested milestone. Never apply a uniform `+0x1010` adjustment to unrelated contributions.
