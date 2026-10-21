# LINK386 R4E — Full source handoff

**Baseline:** R4D-FIX1 (live-proven hello, frozen/immutable) → separate R4E source tree, 2026-10-17.

## Outcome

Two genuine, source-only OS/2 MZ/LE executables have been emitted from unmodified historical C/386 OMF input objects and original `LIBC.LIB` / `OS2386.LIB`:

1. `cmdos2_os2_console_test-r4e.exe`, three application objects; two code objects (32/16), one data/BSS/stack object; entry object1+0x1718; three DLLs; 319 individual loader-fixup sites (255 internal, 64 import).
2. `cmd32os2-r4e.exe`, seven application objects; same three-object geometry; entry object1+0xB9F4; four DLLs; 1,231 individual loader-fixup sites (1,149 internal, 82 import).

The Microsoft LE references were used ONLY by independent **post-build comparison**, not by `link386r4e` or during link output. Their logical page maps match the newly emitted ones, and every mapped initialized code/data byte outside import operands matches exactly for both programs. The output is not expected to be byte-identical to Microsoft LE files due to fixup record packing and header serialization differences.

**STATUS: BUILD-AND-ORACLE-VERIFIED, WINDOWS RUNTIME PENDING.** This is not R4D's runtime proof, which the user already performed successfully and which must remain frozen.

## Source changes (all in new R4E tree)

- `src/link386r4e.c`: three-object LE writer, dynamic LE objects/pages, physical page ordering, import module table, supported loader-fixup records, and `__astart` entry resolution; accepts multiple `.OBJ` and `.LIB` inputs, a DEF file and explicit stack override.
- `src/link386r4e_relocate.c`: mixed-width OMF relocation widths, DLL import-site tracking, 32-bit arithmetic truncation on LP64, `DGROUP` group-relative static fixups. This is a fork of R4D's relocator.
- `src/link386r4e_images.c`: mixed-width masks/source-only image generation; 16:16 and 16:32 fixup widths. Fork of the R4C/R4D materializer.
- `src/link386r4e_layout_core.inc`: canonical C/386 initialization section order (`XI` before `XP`; `CDATA` after them), with existing grouped/combined segment handling. Fork of R4D's layout core.
- `src/link386r4e_def.inc`: small, strict ordinal-import DEF parser; **does not invent PM/VIO/KBD static import LIBs**.
- `src/link386r4e_probe.c` and `src/link386r4e_applyprobe.c`: additional research/diagnostic executables, not required for production.
- `tests/verify_r4e.py`: independent reference oracle, deterministic output test, corrupt-OMF rejection.
- `Makefile`: appended `link386r4e` and `verify-r4e` targets only.

## Debugging history / pitfalls

- The first source-only console layout already matched Microsoft sizes: object1 0x2F16, object2 0x62, object3 0x22A00. Adding DEF bindings resolved 8 references whose DLL imports had no corresponding import library.
- The 16-bit bridge uses 7 selector/far pointer sequences and needs loc=2, loc=3, loc=11 OMF handling, not only loc=9.
- For CODE16's `66 67 EA` jump, the 16:16 pointer stores its object2-relative offset in the first two bytes. Unrelocated values differed from the Microsoft executable until source-side offset writing was added.
- In the full CMD32 case, the `_DATA` and `_TEXT` object extents matched (object1=0xF1D1, object2=0x62, object3=0x51990), but initially data frames `XI` and `XP` were out of sequence. Correcting the initialized class order removed 20 byte mismatches.
- One `strtol.asm` `_errno` fixup uses `frame_method=GROUP(DGROUP)`, which requires a **group-relative data offset**, not preferred absolute virtual address and not a loader OFF32 record. Resolving this removes the final 1-byte difference; all mapped initialized bytes now match.
- `unsigned long` is 64 bits on many Linux builds but a Windows/C386 fixup operand is 32 bits. The correct calculation truncates to 32 bits BEFORE classifying a loader target; otherwise a valid 0x3067 offset can accidentally appear as 0x100003067.
- The CMD32 full data object has four mapped initialized data pages; its remaining BSS/STACK extent is virtual. The full physical page order is **four data pages, sixteen 32-bit-code pages, one 16-bit bridge page**. The console test order is one data page, three 32-bit-code pages, one bridge page. Never use the raw physical page number as the logical object page ordinal.
- The Microsoft full shell's LE metadata describes a larger BSS page count than R4E; the R4E loader scanner/runtime must validate whether the shortened initialized page-map representation is acceptable. No runtime success has been claimed.

## Reproduction and acceptance criteria

From the extracted project root:

    make link386r4e verify-r4e verify-r4d-fix1

Additional GCC and Clang strict-C89 builds compile cleanly and generate **identical resulting LE executable files** for both targets. R4D-FIX1's `src/link386r4d.c` remains byte-identical to the frozen source; its static/oracle tests pass.

**Before freezing R4E as live-proven:** run `os2host32 --scan`, then `--run` on both executable outputs under Windows. The console test waits for one character and should print `M29H_OS2_CONSOLE_BACKEND_OK` on success. On failure, capture the loader log and identify whether the issue is in LE header, page/virtual object sizing, mixed-width fixup type, import module ordinal, or C/386 startup.

**After runtime succeeds:** the next milestone R4F can broaden general-purpose linker options and repeat on SarienPM. If runtime fails, fix the precise issue in a fresh R4E-FIX1 fork, preserve R4E's current source as the forensic baseline, and never modify R4D-FIX1.

## Files preserved

All historical README/HANDOFF files, the original C89 source lineage, frozen R4D-FIX1 reference binaries, current R4E binaries, both source archives, original OBJ files, library inputs, DEF/MAP files, Microsoft reference executables (for independent tests only), and SHA-256 manifest. Build output lives under `build/r4e` and the new top-level R4E executable is `link386r4e`.
