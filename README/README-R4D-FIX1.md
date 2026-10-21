# LINK386 R4D-FIX1 — LE i386/OS2 header encoding correction

**Status:** source-only linker build + structural/oracle verification PASSED; Windows `os2host32` runtime RETEST REQUIRED.

The initial R4D executables were rejected immediately by `os2host32 --scan` with
`os2host32: only i386 LE images are supported`.

## Root cause (proven)

The 16-bit LE header fields at e_lfanew+0x08 and +0x0A were written as
`0x0200` (CPU) and `0x0100` (OS) using a little-endian writer, when the
Microsoft `hi.exe` reference encodes `0x0002` (Intel i386) and `0x0001` (OS/2).
The actual file bytes were `00 02 00 01`; they must be `02 00 01 00`.

Corrected in `src/link386r4d.c` only:

    w16(p+8,0x0002);w16(p+10,0x0001);

Previous R4D source-only output and the corrected output differ at **exactly four
file offsets, 0x98 through 0x9B**. All source-built object bytes, import names,
page maps, and 171 loader relocation site/target tuples remain unchanged.
No Microsoft reference EXE is used in output generation; it is used afterward
as an independent oracle. Frozen R4B/R4C and original R4D are not modified.

## Rebuild

    make link386r4d
    make verify-r4d-fix1
    ./link386r4d -o build/r4d/hi-r4d-source-only.exe fixtures/hi.obj fixtures/LIBC.LIB fixtures/OS2386.LIB
    ./link386r4d -o build/r4d/hi-yo-r4d-source-only.exe fixtures/hi-yo.obj fixtures/LIBC.LIB fixtures/OS2386.LIB

Both GCC and Clang `-O2 -std=c89 -pedantic -Wall -Wextra -Werror` builds pass;
the produced original-hello bytes are identical between compilers.

## Windows test (not run here)

    \OS2\os2host32.exe --scan hi-r4d-source-only.exe
    \OS2\os2host32.exe --run hi-r4d-source-only.exe
    \OS2\os2host32.exe --run hi-yo-r4d-source-only.exe

Expected output if runtime succeeds: `hi!` and `yo!`, respectively.
**Do not consider this live-proven until the user's `os2host32` test passes.**

## Reproducible checksums

See `MANIFEST-R4D-FIX1-SHA256.txt` for archive member checksums. The R4D-FIX1
manifest is separate from the historical R4D manifest, which is retained
unchanged for provenance and is not applicable to the modified files.

Fix1 executable original SHA256: c08ac1e2a88350f28cfffcacb1003be4553863f8131d571667c6882f5e665a24
Fix1 executable modified SHA256: b90d0d43520b7a64edc87a05823e8311480e481aec543029a583b7b415b0528b
