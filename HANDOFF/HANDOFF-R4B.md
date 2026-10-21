# LINK386 R4B: forced, reference-assisted executable reconstruction

**Date:** 2026-10-17. **Baseline:** frozen R4A, copied unchanged to a new R4B tree. R4B adds a bounded C89 relinker, a corrected LE page-map byte-fingerprint audit, reproducible tests and two emitted executables.

## Important status: what did and did not link

**An actual OS/2 LE EXE was emitted.** The byte-for-byte result for the original hi.obj is exactly the Microsoft LINK386 hi.exe: size 11,806, MD5 `b364d870ec760db68317b2e476234e96`, SHA256 `3e0383860d8e76262b3c954bb95ae8e8e3c7887deddfbfd7705c4b8c580441c7`.

**This is NOT a standalone LINK386 replacement yet.** `link386r4b` requires Microsoft's prior successfully linked LE as a trusted scaffold, uses its prelinked LIBC runtime section bytes and existing LE loader-fixup stream, and patches only the application `_TEXT` and `_DATA` contributions from a newly parsed `hi.obj`. It does not independently perform all runtime OMF relocations, place all 39 modules, or create 159+12 loader fixups from scratch. Treat the result as a reference-assisted relink and acceptance experiment; never present the exact hash as proof of independent executable generation.

## Core architecture finding: the LE page map matters

The LE header's page map is a list of logical-page to *physical* file-page numbers, with physical indices encoded as a 24-bit big-endian integer. The original 3 logical pages map to physical file pages **[2, 3, 1]**.

| Logical LE page | Owning object | Physical file page | Contents |
|---|---|---|---|
| 1 | object 1 (code base 0x10000) | 2 | `_main` at VA 0x10010; `crt0.asm` at 0x10020 |
| 2 | object 1 (code base 0x10000) | 3 | runtime and heap `_TEXT`, imports including DOSCALLS |
| 3 | object 2 (RW base 0x20000) | 1 | data, runtime strings, `hi!\n` at VA 0x20044 |

File physical page 3 is truncated to `LastPageSize=0x81E` rather than occupying a whole 4096-byte page. The map was missed in R3V–R3Z fingerprint scripts. This invalidates the earlier inference that heap `_TEXT` resides in LE object 2; on correct virtual mapping the 22 strong `_TEXT` anchors all reside in LE object 1. The apparently leading runtime banner is located in object **2** at 0x20008, not in object 1. True `crt0.asm` begins at **0x10020**, not 0x11020; `heapgrow.c` at **0x112A0**, not 0x202A0.

The output LE fixup decoder's 159+12 counts remain valid, as do the 8 code-object and 4 data-object *loader-fixup site* counts; the mistaken claim was that those four data-object loader references proved heap `_TEXT` ownership.

## The forced linker (`src/link386r4b.c`)

1. Parse hi.obj, LIBC.LIB and OS2386.LIB through original frozen R2 C89 OMF code, including OMF checksum validation and member selection (39 selected modules).
2. Accept only exactly the known good 11,806-byte reference with CRC32 `0x41AE0ED4`; check LE header, objects, page map, data extents, fixup page table and observed source/target forms.
3. Decode the original LE loader fixup stream independently in C. Require exactly **159 internal + 12 imported sites** and verify page bounds. These fixups are *inherited* from the reference, not regenerated.
4. Find the actual application source `.text` contribution by exact 16-byte machine-code fingerprint, ignoring the eight bytes occupied by FIXUPP32 operands. Find the `_DATA` string contribution in object 2.
5. Overlay the application `LEDATA32` bytes from the freshly parsed OMF object and **apply both real OMF fixups**: absolute 32-bit `_DATA` pointer at `0x10011 -> 0x20044`, PC-relative call at `0x10016 -> _printf` at `0x10568` (relative displacement `0x54E`). `_printf` is provided by the selected LIBC.LIB but its final address comes from the audited scaffold call target, not an independent layout pass.
6. Verify the two resulting fixup fields agree byte-for-byte with Microsoft's original for an unchanged object. Save the LE. No new loader-fixup records are created.

`--allow-data-edit` permits a five-byte data literal edit at the already-validated pointer target. Without it, the source data signature must be found unchanged in the reference.

## Reproducible commands

```sh
make link386r4b verify-r4b
./link386r4b -r fixtures/hi-reference.exe -o build/hi-r4b.exe fixtures/hi.obj fixtures/LIBC.LIB fixtures/OS2386.LIB
./link386r4b --allow-data-edit -r fixtures/hi-reference.exe -o build/hi-yo-r4b.exe fixtures/hi-yo.obj fixtures/LIBC.LIB fixtures/OS2386.LIB
python3 tests/r4b_corrected_order.py > build/corrected-order.txt
make verify-r3s verify-r3u verify-r4a verify-r3z
```

GCC and Clang both compile with `-std=c89 -Wall -Wextra -Werror -pedantic`; both emitted identical hi.exe bytes. New verification includes:

- unmodified hi.obj -> exact Microsoft reference bytes and hash;
- source-changed (checksum-correct) `hi-yo.obj` -> two altered executable data bytes, preserving 171 loader fixup sites;
- editing denied without explicit opt-in;
- rejected corrupted reference and corrupted OMF checksum;
- corrected all-22 fingerprint runtime anchor mapping.

The altered file's data bytes at VA 0x20044 are `yo!\n\0` and its SHA256 is `980e66aecbecf9237d36c0dfa6bdcdd8ed119e37701643020d82ee8ee4feb4aa`. This proves the tool actually consumes source OMF content rather than blindly copying the reference. No OS/2 runtime execution of the altered file was performed here; expected behaviour is not an execution test.

## Next engineering stage: an **independent** linker

Start from the corrected page map and the recorded 39-module selection closure. Fix source record attribution (crt0 LIDATA coordinate 13), external-index semantics (`heapgrow.c` index 13 vs 12), and correctly determine `_end` + 0x400. Derive exact module/segment/data placement from OMF, not the LE reference. Implement whitelisted runtime FIXUPP32 static writeback, LE object/page construction, and regeneration of all 159 internal + 12 import sites. Only when `hi.exe` can be emitted without `-r` should it count as a standalone LINK386 replacement. Do not treat the current R4B tool as a general linker or extrapolate the trusted-reference placement to CMD32/SarienPM.

## File inventory

- `src/link386r4b.c`: new portable reference-assisted relinker.
- `tests/verify_r4b.py`: source-change and negative acceptance tests.
- `tests/r4b_corrected_order.py`, `build/corrected-order.txt`: corrected virtual-page fingerprint evidence.
- `fixtures/hi-yo.obj`: checksum-correct input variant.
- `build/hi-r4b.exe`, `build/hi-yo-r4b.exe`: emitted executables.
- All prior R4A C sources, original fixtures, reference binaries, tests, reports and handoffs remain included.

**Safety boundary:** No guessing, hardcoded illegal relocation patches, synthetic missing symbols or silent fallback to unsupported code. If source shape or reference fingerprint differs, the bounded relinker refuses the operation.
