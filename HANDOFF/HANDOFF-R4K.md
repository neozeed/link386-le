# LINK386 R4K — SarienPM full mapped-image and LE-fixup convergence

## Status — BUILD / REFERENCE VERIFIED, RUNTIME UNTESTED

R4K is a new, separate source tree based on R4J. R4D-FIX1 and R4E-FIX1
remain frozen and live-proven on Windows/os2host32; they were NOT modified.
Do not label R4K live-proven until it has successfully run under soft386/os2host32.

R4K independently links the supplied 42 Sarien C/386 objects, LIBC.LIB,
OS2386.LIB and sarien.def. It does not read the original Microsoft-generated
sarien.exe to generate the output executable. HOWEVER, the opt-in R4K placement
manifest and COMDEF ordering were RECOVERED by comparing the Microsoft reference
with earlier linker outputs. This is a reference-guided Sarien *profile*, not
an independent discovery of general-purpose Microsoft LINK386 ordering rules.

## Result against original Microsoft Sarien executable

- R4J ordinary code differences: 9,344; R4K: **0**.
- R4J ordinary initialized data differences: 799; R4K: **0**.
- R4J LE loader relocation site/type/target exact matches: 1,421 / 2,789.
- R4K exact LE loader relocation sites/types/targets: **2,789 / 2,789**.
- R4K *entire mapped object images* compare BYTE-FOR-BYTE identical with
  Microsoft: object 1 = 74,844 bytes; object 2 = 392,368 bytes.
- R4K LE executable file: 113,756 bytes (not equal to Microsoft's 107,612).
  Our writer uses individual fixup records rather than Microsoft source-list
  compression and retains its existing physical/virtual page metadata policy.
- Reference executable is only opened by the POST-LINK verification test.
- The R4K generated executable has NOT been run by the builder under Win32.

## Technical corrections

1. Recovered the object-1 contribution ordering *per segment*, not per library
   module. The generated `src/r4k_order.inc` gives offsets for 104 original
   code contributions, derived from relocation-masked byte searches and gap
   constraints. Unlike R4J, this correctly separates two distinct `_TEXT`
   contributions of `memcpy.asm` and handles small intervening CRT segments.
2. Solved object-2 `_DATA` ordering by OMF lengths, alignment and reference
   anchors: `_file.c` follows `fclose.c`; `ctype.asm`, `strtok.asm`, `assert.c`,
   `crt0msg.asm`, `stdargv.asm`, `_sftbuf.asm`, `output.asm`, `heapinit.c`,
   `chkstk.asm`, `_cflush.asm`, `cmiscdat.asm`, `days.c`, `timeset.c`, `tzset.c`,
   `txtmode.asm` occupy a contiguous sequence. Ordered MSG records as
   `abort.asm`, `crt0msg.asm`, `crt0fp.asm`, `hpabort.asm`.
3. Reconstructed 34 COMDEF communal allocation slots, including `_Bitmap`
   (64,000 bytes) and `_game` (55,324 bytes), from target offsets. `_game`
   alone accounted for 962 wrong relocation targets in the old release (all
   displaced by 184 bytes). Allocation in the R4K-only path is reordered to
   match the reference; the earlier compatibility path is untouched.
4. Fixed OMF `LEXTDEF`/`LPUBDEF` scope in the R4K path. `sprite.c` and
   `view.c` contain local function names also defined globally or elsewhere.
   Resolving their calls globally left seven static PC-relative operands
   incorrect, despite the loader fixup inventory matching. The OMF parser now
   retains local/public provenance, and R4K resolves module-local publics first
   for local externals. This removed the last 14 differing mapped bytes.

## Verification

- GNU C Compiler strict C89, -pedantic -Wall -Wextra -Werror: PASS.
- Clang strict C89: PASS.
- GCC and Clang generated Sarien executable SHA256: identical.
- Two separate Sarien builds deterministic.
- Strict input isolation: R4K test copies the 42 objects, two libraries and
  DEF into a temporary input folder, with **no Microsoft EXE present**, invokes
  the linker, and only subsequently reads the Microsoft reference for comparison.
- All 2,789 LE relocation source/site/type/target records exactly match.
- All mapped object bytes exactly match.
- Frozen R4D-FIX1 and R4E-FIX1 regression suites pass.

## Reproduce on Linux

```
make link386r4e
make verify-r4k
make verify-r4e-fix1 verify-r4d-fix1
bash build/r4k-command.sh
```

Use `build/r4k-command.sh` for the complete 42-object argument list. Use
`rebuild-r4k.cmd` from Windows/MinGW. The `--r4k-evidence-order` flag activates
only this experimental Sarien-specific profile; the default linker behaviour
and frozen executable outputs retain their earlier semantics.

## Next test

Windows:
```
\OS2\os2host32.exe --scan sarien-r4k.exe
\OS2\soft386_os2.exe sarien-r4k.exe
```

Compare palette, PM window, game-script progress, and whether the guest halts
without DosExit; runtime is presently UNTESTED. On an unexpected halt, capture
EIP/EFLAGS/registers and stack, and compare against known-good Microsoft Sarien.

## Limitations and next work

This is not yet a general-purpose linker-ordering implementation for arbitrary
OMF libraries. The Sarien-specific evidence-derived placement manifest must
be replaced with a principled archive-selection / segment-order algorithm.
LE loader record grouping and physical/virtual page table optimization are not
byte-for-byte Microsoft compatible. Runtime success needs user validation.
