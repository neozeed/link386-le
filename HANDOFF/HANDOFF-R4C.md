# LINK386 R4C — Independent OMF runtime object construction

Date: 2026-10-17. Baseline: frozen, LIVE-PROVEN R4B. No changes are made to the frozen R4B archive or its source tree.

## Objective and achieved boundary

**Achieved:** Generate the complete virtual byte extents of the C/386 hello executable's two LE objects **without consuming the Microsoft reference EXE**. The R4C code reads the actual `hi.obj`, `LIBC.LIB` and `OS2386.LIB`, selects 39 members, expands OMF LEDATA/LIDATA, coalesces segments, honours group information, produces zero-filled BSS/stack, and emits an original, explicitly unrelocated byte stream per object. Both object sizes match Microsoft: object 1 = 6,174 (0x181E), object 2 = 10,256 (0x2810).

**Not achieved:** A self-contained runnable LE executable. There is no independent MZ/LE header/page/import/fixup writer or trustworthy application/runtime relocation writeback yet. Reference-assisted R4B is still the only runnable emitted-executable milestone.

## Verification results

* 39 selected modules, 120 segment contributions, 301 recorded OMF fixups, 8 OS2386 import definitions.
* Materialized object 1: 6,174 bytes; 4,968 ordinary source bytes; 1,168 relocation-marked bytes; 38 zero/unclaimed bytes. **All 4,968 ordinary bytes and all 38 unclaimed zero bytes agree exactly with Microsoft hi.exe** when checked after the build using the correct logical->physical LE page mapping [2,3,1]. This includes the entire C runtime code order, not just fingerprints.
* Materialized object 2: 10,256 bytes; 1,290 ordinary source bytes, 36 relocation-marked bytes and 8,930 zero/unclaimed bytes. All 8,930 unclaimed bytes agree with the zero-filled reference virtual image. **13 ordinary source byte positions differ**: offsets `0x58,0x59,0x5A,0x3F0,0x3F1,0x3F4,0x3F5,0x3F8,0x3F9,0x3FC,0x401,0x404,0x405`. Do not suppress these. The first three fall in crt0.asm `_DATA`; the latter ten in cmiscdat.asm `_DATA`.
* Independently evaluated 284 of 301 candidate fixups. Of these 284 candidate four-byte fields, 223 match Microsoft exactly, 61 differ. The other 17 are 12 DLL imports and 5 unresolved/unsupported targets (three unresolved external references, one linker boundary and one out-of-range external index). They are not linked and are never silently skipped in the relocation report.
* Editing the `hi.obj` text from `hi!\n` to `yo!\n` changes precisely the two corresponding source-image bytes (`object2` offset 0x44/0x45). The original reference EXE is not an input to either materializer.
* GCC/Clang strict-C89 builds passed. Repeated builds are byte-identical. Tampering with an OMF record checksum is rejected. Frozen R4B `make verify-r4b` passed on the R4C tree with the LIDATA correctness fix.

## Critical corrections to the older linker

1. **R3L's group-key restriction was too strong for the historical C/386 hello `_DATA` layout.** R4C has a profile-limited combination exception for same-name, same-class `_DATA` contributions in its private `link386r4c_layout_core.inc`. GROUP membership must be retained for fixup frame calculation, but does not invariably prevent matching segments from coalescing. This adjustment reduced regular data mismatches sharply while preserving all other R3G/R3L baseline files untouched.
2. **OMF COMMON combine=6 overlays are not ordinary concatenations.** For the hello profile, the longest PAD contribution (hpabort.asm 19 zeros) is the winning initialized contribution over multiple four-byte `FF` PAD candidates. R4C materializes that extent and confirms it byte-for-byte against Microsoft. Do not extrapolate this rule to arbitrary COMBINE=6 data without verification.
3. **A real LIDATA32 expander memory bug was corrected.** In the inherited `src/link386r2.c`, repeated iterated-data blocks used a pointer into a buffer that `iter_append` could `realloc`, leaving a dangling source pointer. R4C copies the repeated unit into a stable scratch buffer before appending; a two-byte nondeterminism in `_file.c` disappeared. Keep this fix for future linker versions.
4. **LE physical page order is [2,3,1].** `hi!` is at virtual `0x20044`, `_main` at `0x10010`, crt0 at `0x10020`, and heap `_TEXT` contributions are in object 1, *not* object 2. The code-object offset `0x10` is explicitly a verified **hello profile** leading prefix; it is not a universal OMF start offset.

## Source and experiments

* `src/link386r4c.c`: C89 segment-by-segment materializer and data/fixup/symbol manifests; no reference file argument.
* `src/link386r4c_images.c`: C89 output-object materializer; emits `object1-unrelocated.bin`, `object2-unrelocated.bin`, and byte-accurate `object1-mask.bin` / `object2-mask.bin` (0=not sourced / BSS, 1=plain source byte, 2=unapplied FIXUPP operand).
* `src/link386r4c_layout_core.inc`: R4C-private, data-merge variation of the old R3G planner, leaving frozen R3G unmodified.
* `src/link386r4c_relocate.c`: **diagnostic only**; emits candidate-relocated object images and `relocation-candidates.tsv` for all 301 input fixups. Those images have 61 mismatching candidate relocation fields, 12 unlinked import references, five other deferred fixups and 13 plain data discrepancies. **Do not execute or package them as runnable LE.**
* `tests/verify_r4c.py`: compares source-only assembled images against Microsoft oracle *after* image generation; tests deterministic repeat build, changed-source content, corrupted input, and relocation candidate match/defer counts.
* `tests/r4c_code_reconstruct.py`: independent Python cross-check that concatenating all selected `_TEXT` segments in OMF selection order plus profile prefix+alignment produces the exact source code object.
* `tests/r4c_compare.py`: optional fingerprint corroboration with the correct LE page map.
* `build/r4c/`: generated segment bytes, 2 object images + masks, original reports, independent oracle comparisons, candidate relocation images and outcomes.

## Reproduce

```sh
make link386r4c link386r4c_images link386r4c_relocate
make verify-r4c verify-r4b
./link386r4c build/r4c fixtures/hi.obj fixtures/LIBC.LIB fixtures/OS2386.LIB
./link386r4c_images build/r4c fixtures/hi.obj fixtures/LIBC.LIB fixtures/OS2386.LIB
./link386r4c_relocate build/r4c fixtures/hi.obj fixtures/LIBC.LIB fixtures/OS2386.LIB
python3 tests/r4c_code_reconstruct.py build/r4c
```

## Next phase (R4D recommendation)

Reconcile external-symbol index interpretation and compiler/library weak-symbol behavior, especially `fflush.c`, `heapadd.c`, `heapgrow.c` and synthetic CRT symbols, without patching a generalized rule on the strength of the hello case alone. Resolve 13 residual data bytes from their OMF linker directives/special contributions; then validate all generated relocation fields, regenerate LE loader internal/import fixups, and emit the header and logical/physical page tables without `-r`. Only a successful clean Windows os2host32 launch of an LE produced without the reference scaffold should be described as an independent LINK386 executable link.
