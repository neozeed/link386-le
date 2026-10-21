# LINK386 R4E-FIX1

This correction was prompted by live Windows loader output: `unknown/undefined LX source fixup type` for both R4E CMD32 programs. A malformed LE selector record added an illegal target-offset word; 16:16 bridge fixups also needed selector-only alias encoding, and one full-shell internal DWORD required a cross-page duplicate fixup.

See **HANDOFF-R4E-FIX1.md** for the exact record-level evidence and verification counts.

Run `make verify-r4e-fix1 verify-r4d-fix1` on Linux, or `rebuild-r4e-fix1.cmd` on Windows. The R4E-FIX1 binaries are under `build/r4e-fix1/console/` and `build/r4e-fix1/full/`.

**Windows runtime status: pending user test**. Frozen R4D-FIX1 hello remains live-proven; frozen R4E remains unchanged. This corrected branch has not yet been live-tested under os2host32.
