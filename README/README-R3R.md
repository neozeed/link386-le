# LINK386 R3R — OMF coordinate and namespace reconciliation

R3Q is frozen unchanged. R3R is a separate reproducible tree that adds `tests/r3r_reconcile.py` and a `make verify-r3r` target. The core C89 planner remains unchanged: the evidence does **not** justify patching it yet.

## Independent raw-record findings

* `crt0.asm`: Four LIDATA32 records in segment 2 expand to four bytes apiece, at offsets 0, 4, 8, and 12. Its following FIXUPP32 explicitly gives location kind 9 and coordinate 13. A conventional four-byte write is invalid against the final four-byte record under both record-relative (13+4 > 4) and segment-absolute (13+4 > 16) interpretations. The subsequent LEDATA32 segment-2 record starts at offset 16, so a write spanning two records **might** be intentional, but this is not proof. Do not patch until we verify location-kind 9, the source-address convention, and any cross-record byte-spanning semantics.
* `heapgrow.c`: Independent namespace accounting for EXTDEF (0x8C), LEXTDEF (0xB4), COMDEF (0xB0), and LCOMDEF (0xB8) gives 12 ordered indices. Its FIXUPP32 explicitly references external index 13, outside that namespace. No missing COMDEF/LCOMDEF entries exist in this module. This remains a genuine unexplained historical inconsistency or decoder-semantic issue, not grounds for manufacturing index 13.
* `chkstk.asm`: `_end` still requires a validated final boundary address; R3R does not assign one.

`r3r-reconciliation.txt` records the raw findings. It is deterministic and verified against original `LIBC.LIB` fixture; no LE output or relocation writebacks are claimed.

## Run

```
make link386r3o verify-r3r
make verify verify-r3g verify-r3i verify-r3j verify-r3k verify-r3l verify-r3m verify-r3n verify-r3o verify-r3p verify-r3q
```
