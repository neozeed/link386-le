# LINK386 R3Q — independent OMF FIXUPP thread audit

R3P is frozen unchanged. R3Q is a separate tree adding `tests/r3q_thread_audit.py`, a raw-record FIXUPP/FIXUPP32 decoder independent of the R2/R3O in-memory decoder. `make verify-r3q` checks the original `fixtures/LIBC.LIB` and writes `r3q-thread-audit.txt`.

## Verified byte-level findings

* `heapgrow.c`: the initial FIXUPP32 at module offset 0x94 initializes target threads 0..3 to SEGDEF indices 3, 2, 1, 4, and frame threads 0..1. The later FIXUPP32 at 0x432 includes a directly encoded target method 2 (external) and external index 13 at record-relative byte 44, source offset 378, against twelve EXTDEF entries. **This is not an uninitialized-thread accident**. The raw record itself explicitly encodes index 13. The source of the thirteenth index is still unknown; no fake entry was added.
* `crt0.asm`: consecutive LIDATA32 records at 0x26C, 0x286, 0x2A0, 0x2BA have segment 2 starting at 0, 4, 8, 12 respectively, and the FIXUPP32 record at 0x2D4 contains source coordinate 13. The last LIDATA's encoded data payload is 17 bytes; expanded output is smaller. This strongly suggests the R2 decoder's assignment of the source coordinate within the final expanded block needs further investigation, but does **not** prove whether that coordinate is segment-absolute or tied to a shared iterated-data logical record.
* `chkstk.asm`: the FIXUPP32 at 0xB0 encodes external index 1 (`_end`) plus displacement 0x400, with source coordinate 0. The outstanding step is determining the final linker boundary address, not recovering the external symbol name.

## Scope / limitations

R3Q adds independently checked raw-FIXUPP provenance and regression coverage. It **does not** change relocation semantics, patch executable bytes, resolve the unknown heapgrow index, or emit a linked LE executable. The older planner and test suite remain unmodified. No artificial padding or speculative symbol is introduced.

Run `make verify-r3q verify-r3p` and optionally `make verify verify-r3g verify-r3i verify-r3j verify-r3k verify-r3l verify-r3m verify-r3n verify-r3o`.
