# LINK386 R4D-FIX1 — corrected independent LE writer handoff

## Summary

The user ran the independent R4D `hi-r4d-source-only.exe` and `hi-yo-r4d-source-only.exe`
under Windows `os2host32 --scan`/`--run`, which rejected both with
`only i386 LE images are supported`. Inspection against the user's byte-identical
Microsoft-produced `hi.exe` revealed an LE header CPU/OS byte-order bug.

- Microsoft LE: `e32_cpu=2`, `e32_os=1`, file bytes `02 00 01 00` at LE+8.
- Original R4D: `e32_cpu=0x0200`, `e32_os=0x0100`, bytes `00 02 00 01`.
- FIX1: `e32_cpu=2`, `e32_os=1`, bytes `02 00 01 00`.
- File offsets changed exactly `0x98..0x9B` (MZ LE header position `0x90`).
- Full original R4D image and LE loader fixup-site verification still passes.
- GCC and Clang executable bytes match.
- Windows runtime scanner and execution verification pending user retest.

## Test files

- `build/r4d/hi-r4d-source-only.exe` — fixed hi
- `build/r4d/hi-yo-r4d-source-only.exe` — fixed yo
- `tests/verify_r4d_fix1.py` — checks LE CPU and OS independently of the older image/fixup oracle

## Scope and limitations

This is a targeted correctness fix to R4D's LE header emitter, not a new general
LINK386 workalike. It does not address potential later os2host32 loader or runtime
issues, which can only be established by the live retest. If the scanner reaches
another error, use its exact diagnostic and compare field-by-field to the original
Microsoft reference. Preserve R4B, R4C, and earlier packages.

For R4D architectural detail, see `HANDOFF-R4D.md`.
