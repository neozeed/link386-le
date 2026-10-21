# LINK386 R3P — original-byte OMF provenance audit

Baseline: frozen R3O. **This is a diagnostic milestone**. No fixup writebacks, new LE link, or speculative target insertion.

New `tests/r3p_raw_audit.py` independently scans the original LIBC.LIB archive and displays full original-byte EXTDEF tables and the DATA/FIXUPP timeline for `crt0.asm`, `chkstk.asm`, and `heapgrow.c` (see `r3p-raw-omf-audit.txt`). It does not depend on the existing C OMF parser.

Established from original records:

* `crt0.asm`: 21 explicit external names and a FIXUPP32 record immediately after four consecutive LIDATA32 records. The final record's raw byte position and full payload are captured. The R3O association of fixup offset 13 with a last expanded block of four bytes still needs a precise definition of LIDATA FIXUPP location numbering; do **not** simply add 13 to that four-byte block.
* `chkstk.asm`: `_end` is the first explicit external and is a linker-synthesized boundary; exact placement remains unverified.
* `heapgrow.c`: two EXTDEF records contain 1 + 11 = **12** explicit names, independently confirming the parser's external count. The last is `__amblksiz`. The R3O diagnostic's reference to external index 13 is **not** explained by a lost EXTDEF. Next investigate FIXUPP thread-bit interpretation / fixdat indexing and record-local previous thread state using its raw hex payload. Do not append a phantom 13th name.

Run:

```
make link386r3o verify-r3p
python3 tests/r3p_raw_audit.py fixtures/LIBC.LIB
```

This incremental audit intentionally preserves the R3O linker implementation unmodified; an evidence-based parser fix remains future work. R3N candidates are not validated against Microsoft's emitted LE bytes.
