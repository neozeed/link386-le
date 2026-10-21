# LINK386 R3O — bounded relocation-blocker provenance

Frozen baseline: R3N. R3O is a standalone *diagnostic* successor, **not** an LE linker.

`link386r3o` preserves R3N's candidate calculations, and adds evidence-focused `DEFER` reporting for unresolved relocation bookkeeping. Unlike a true validation step, this release does **not** prove any candidate fixup value against corresponding bytes in the Microsoft LE reference. It makes no relocation writebacks.

## Hello triage

* `crt0.asm` fixup 1: `SITE_OUTSIDE_DATA`, target `__exit`, target external index 19 of 21; associated data starts at offset 12, contains 4 bytes, fixup at offset 13. This is an attribution/OMF record association question, **not** proof that an arbitrary padding fix is correct.
* `chkstk.asm` fixup 1: `TARGET_EXTERNAL_UNRESOLVED`, target `_end`, index 1 of 2; this is linker-synthesized but has no final, validated address in R3N. It must remain deferred.
* `heapgrow.c` fixup 12: `TARGET_EXTERNAL_INDEX_INVALID`, index 13 of 12. Investigate FIXUPP external-symbol numbering across LEXTDEF/COMDEF and the actual binary record before modifying parser semantics.

Summary for `hi.obj LIBC.LIB OS2386.LIB`: total=301, candidate_offset32=286, imports=12, deferred=3, blocked=0, writebacks=0. **Zero `blocked` here is a classification change, not a fix.** The original R3N candidate values are not validated for applying to output bytes.

## Reproduction

```
make link386r3o verify-r3o
./link386r3o fixtures/hi.obj fixtures/LIBC.LIB fixtures/OS2386.LIB
```

The next step is an original-OMF byte-level audit of the three suspect fixups and an LE oracle comparison of selected known-good displacement values. Only then should concrete writebacks be enabled.
