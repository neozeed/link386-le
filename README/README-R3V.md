# LINK386 R3V — Eight independently corroborated code import sites

Baseline: **R3U frozen**, unchanged. R3V is a separate copy and contains an independent read-only cross-check. No linker executable-generation logic or runtime relocation writer has changed.

## Confirmed

All eight code-object DOSCALLS fixup sites in R3N have the same displacement **+0x1010** compared with the real Microsoft LE output. The oracle pairing is made by DOSCALLS ordinal, and repeated calls to an ordinal require matching all the expected addresses. `tests/verify_r3v.py` asserts all eight. Independent raw LEDATA fingerprints from the original Microsoft `LIBC.LIB` identify corresponding `_TEXT` segment contribution bases in LE object 1 for `crt0.asm`, `crt0dat.asm`, `nmsghdr.asm`, `lseek.asm`, and `write.asm`. The signatures are matching immutable 12-byte windows; no relocated bytes are assumed to be equal.

This establishes a real code-contribution placement shift for the tested cases; **do not generalize the +0x1010 shift to other source segments**.

## Still unresolved

The four Microsoft LE data-object loader sites are at `0x20374` (DosAllocMem 299), `0x20456` (DosSetMem 305), `0x204AE` (DosFreeMem 304) and `0x204FF` (DosAllocMem 299). Our earlier classifier put the corresponding four references in the code object. Correlation with precise OMF source data contributions is still necessary. The matching bytes in `_TEXT` of `heapgrow.c` or `heapinit.c` do not automatically establish those four import source sites.

The three disputed `crt0.asm`, `chkstk.asm` and `heapgrow.c` records remain deferred. No general relocation evaluation has been validated from this test and no new hi.exe was linked.

## Reproduce

`make verify-r3v` emits `r3v-import-provenance.txt`. GCC/Clang and previous R3U checks are preserved. Test fixtures are the supplied original hi.exe, LIBC.LIB, OS2386.LIB.
