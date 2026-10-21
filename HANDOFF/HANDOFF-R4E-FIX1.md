# LINK386 R4E-FIX1 — mixed-width LE fixup serialization correction

## Status and frozen baselines

- R4D-FIX1 is **FROZEN, LIVE-PROVEN** on Windows/os2host32 (`hi!`, `yo!`). It remains untouched.
- R4E is **FROZEN, BUILD/ORACLE-VERIFIED** (console/full CMD32 output); its first Windows test failed `unknown/undefined LX source fixup type` after successful header/object/import-table scanning.
- R4E-FIX1 is a separate branch from R4E. Its corrected CMD32 EXEs have passed GCC/Clang builds and independent loader-fixup comparisons. **Windows execution remains UNTESTED** in this environment.

## Root causes (verified against the original Microsoft LE files)

1. **Selector source type `0x02` has no target-offset field.** Original R4E serialized an additional two-byte `TRGOFF` after the target object number. A conforming reader treats those two bytes as the start of the following fixup record, misparses its source type, and rejects the image. R4E-FIX1 omits these bytes.
2. **Internal 16:16 pointer OMF fixups are resolved statically except for the selector word.** Microsoft's emitted LE records fix up the selector at pointer site **+2**, using source type `0x02` with the alias flag (`0x12`, or `0x32` when represented as a source list). R4E emitted unsupported/unnecessary full 16:16 internal pointer records (`0x03`) instead. R4E-FIX1 maps internal `loc=3` to selector-only alias, while retaining import 16:16 alias fixups (`0x13`) and internal 16:32 pointer fixups (`0x06`).
3. **Cross-page fixups require records in both page streams.** A full-CMD32 internal DWORD at object-1 offset `0x2FFE` crosses a 4 KB page boundary. Microsoft represents it in both adjacent fixup pages, the latter with a signed negative source offset (`-2`). R4E emitted only one record. R4E-FIX1 emits both and computes fixup page ownership from the effective site address, including 16:16 selector `+2` cases that cross a page boundary.

The original Microsoft binaries use source-list records to share targets. R4E-FIX1 deliberately continues to use simple one-site records, which are valid LE encodings and preserve the exact semantic site/target set. This is why files are larger than Microsoft output.

## Independent oracle results

Both outputs are source-only: `link386r4e` consumes the original `.OBJ`, `LIBC.LIB`, `OS2386.LIB`, and `.DEF` inputs, **never the Microsoft executable**. Reference EXEs are read after generation only by test code.

| Profile | Microsoft fixup records/sites | R4E-FIX1 records/sites | Semantic missing/extra |
| --- | ---: | ---: | ---: |
| CMD32 console | 149 / 319 | 319 / 319 | 0 / 0 |
| Full CMD32 | 803 / 1232 | 1232 / 1232 | 0 / 0 |

The comparison normalizes each fixup to `(object, object offset, source type, alias, target kind, DLL/object, ordinal/offset)` and expands Microsoft's source-list records. The extra full-shell site relative to earlier R4E measurements is a required duplicate cross-page loader record. All initialized object bytes match Microsoft outside deferred imports; no executable scaffold is used.

- Console EXE: 19,554 bytes; entry object 1+0x1718; DLLs DOSCALLS/VIOCALLS/KBDCALLS.
- Full CMD32: 91,746 bytes; entry object 1+0xB9F4; stack object 3+0x51990; DLLs DOSCALLS/VIOCALLS/KBDCALLS/SESMGR.

## Rebuild, retest, and next diagnosis

Linux:

```sh
make link386r4e verify-r4e-fix1 verify-r4d-fix1
python3 tests/verify_r4e_fixups.py
```

Windows MinGW: run `rebuild-r4e-fix1.cmd`.

Windows runtime acceptance:

```bat
\OS2\os2host32.exe --scan cmdos2_os2_console_test-r4e-fix1.exe
\OS2\os2host32.exe --run cmdos2_os2_console_test-r4e-fix1.exe
\OS2\os2host32.exe --scan cmd32os2-r4e-fix1.exe
\OS2\os2host32.exe --run cmd32os2-r4e-fix1.exe
```

The console test waits for a keystroke and should print `M29H_OS2_CONSOLE_BACKEND_OK` on success.

**If the scanner still fails**, obtain the byte position and source type of the first rejected record from os2host32, rather than changing working OMF relocation equations or guessing. If scanning succeeds but execution fails, audit selector/pointer runtime fixups and the VIO/KBD/SESMGR service layer separately. Mixed 16/32-bit runtime execution is not proven just by a successful LE scan.

## Source deltas

- `src/link386r4e.c` — selector record width, selector-site conversion, cross-page duplicate fixup support.
- `tests/r3s_le_fixups.py` — spec-compliant internal-selector decode (no target-offset field).
- `tests/verify_r4e_fixups.py` — independent Microsoft/reference comparison and strict site/target inventory.
- `Makefile` — `verify-r4e-fix1` target.
- `rebuild-r4e-fix1.cmd` — MinGW commands.
- `HANDOFF-R4E-FIX1.md` — this document.

Original archived R4D-FIX1 and R4E source packages were not changed.
