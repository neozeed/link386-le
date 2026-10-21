# LINK386 R4F — Experimental SarienPM source-only link

Fork of frozen LIVE-PROVEN R4E-FIX1. Consumes 42 original application OBJs,
LIBC.LIB, OS2386.LIB and sarien.def; no Microsoft sarien.exe is used during
construction. The original Microsoft executable is only a post-build oracle.

Generated: `build/sarien-r4f.exe` (EXPERIMENTAL, NOT LIVE-PROVEN, known
byte and virtual-page-table mismatches). Don't replace working SarienPM.

Run `make link386r4e`, execute `build/command.txt`, then
`python3 tests/verify_r4f.py` and `python3 tests/r4f_compare.py`.
Regressions: `make verify-r4e-fix1 verify-r4d-fix1`.

Read HANDOFF-R4F.md and R4F-BYTE-COMPARE.txt for unresolved items.
