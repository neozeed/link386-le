# LINK386 R3T — OMF candidate ↔ Microsoft LE relocation-site correlation

Baseline: frozen R3S (unchanged); this separate R3T working tree adds `tests/r3t_correlate.py` and `tests/verify_r3t.py`.

`make verify-r3t` regenerates the independent output `r3t-correlation.txt`. The reference is the original Microsoft `hi-reference.exe` (11,806 bytes), and the input-side predictor is frozen `r3n-hi-report.txt`.

## Verified observations

* Original LE reference has 78 fixup records with 159 internal and 12 imported relocation sites, 171 total.
* R3N reports 286 static candidate coordinates and 12 import candidate coordinates, 298 total.
* The two **raw coordinate sets have zero exact matches**, including zero import-site matches. This does not imply 298 erroneous relocations: static OMF fixups may be resolved and omitted from LE loader fixups entirely. It does mean this candidate layout has not passed the reference-site acceptance gate.
* R3N's report is not an exact Microsoft contribution-placement oracle; there is no legitimate basis for applying a single global offset correction or inventing relocated bytes.
* The `crt0.asm`, `chkstk.asm`, and `heapgrow.c` cases remain unresolved. R3T correlates their existing candidate coordinates, not the original ambiguous record semantics. No unverified FIXUPP handling was added.
* Some LE sites reside in the final mapped page beyond an object's logical size; their offsets remain part of the exact LE fixup stream. Range checking uses *mapped page extent*, not unrounded logical size.

## What is not proven

No binary emitted, no relocated LE assembled, no exact OMF→LE site mapping established, no linker C source changed and no emulator execution claimed. The next step must recover authoritative per-contribution placement (preferably byte/segment alignment against the reference output) before any relocation writeback.

## Reproduce

```
make verify-r3t
cat r3t-correlation.txt
```
