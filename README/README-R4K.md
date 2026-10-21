# LINK386 R4K — SarienPM exact mapped-image candidate

R4K reconstructs Sarien from its 42 Microsoft C/386 OBJ files and original
LIBC.LIB / OS2386.LIB, without consuming a prelinked EXE during the link.
The source is portable strict C89. R4K uses a **reference-derived placement
profile** enabled by `--r4k-evidence-order` (Sarien only); it is not yet a
general Microsoft LINK386 replacement. See HANDOFF-R4K.md.

To run all static tests: `make verify-r4k verify-r4e-fix1 verify-r4d-fix1`.
Build manually: `bash build/r4k-command.sh` (Linux), `rebuild-r4k.cmd` (MinGW).

Static result: Microsoft's 74,844-byte code and 392,368-byte data/BSS mapped
objects match exactly, and all 2,789 LE fixup sources and targets match.
Runtime status: **not yet tested on Windows**.
