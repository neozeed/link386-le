# LINK386 R4E — CMD32 source-only LE linker (experimental)

**Current milestone: see [README-R4E.md](README-R4E.md) and [HANDOFF-R4E.md](HANDOFF-R4E.md).**

R4D-FIX1 is the immutable, Windows-live-proven hello baseline. R4E is a separate multi-object experiment; both CMD32 executables pass build/oracle checks, but have NOT yet been live-tested on Windows.

---

# LINK386 R3E — Synthetic C/386 boundary symbol resolution

R3E advances R3D's portable ISO C89 OMF linking planner. It recognizes the Microsoft C runtime linker-defined `_edata` and `_end` symbols in global external resolution and lazy archive selection. These names are **not ordinary library exports** and do not trigger library-member extraction. The planner now reports them distinctly as `RESOLVED synthetic` with `VA=DEFERRED`, plus a boundary reference count. No fake segment offsets or LE object addresses are assigned.

The existing R3D global COMDEF/LCOMDEF parsing, largest-size communal coalescing, strong-definition overrides, and tentative aligned common-BSS plan remain intact. `PMWIN`, `QUECALLS`, `PMGPI` and `DOSCALLS` refer to runtime DLL imports and are not additional input archives.

## Build and regression tests

```sh
make verify
./link386r2 fixtures/hi.obj fixtures/LIBC.LIB fixtures/OS2386.LIB > hello-plan.txt
./link386r2 fixtures/sarien/*.obj fixtures/LIBC.LIB fixtures/OS2386.LIB > sarien-plan.txt
```

`make verify` exercises the R2/R3/R3A/R3C/R3D fixtures and new R3E boundary-symbol assertions. It verifies 39 selected hello modules, 172 selected Sarien modules, zero unresolved external occurrences for both, and explicit deferred addresses for `_edata`/`_end`. The R3D assertion script was updated to expect R3E's changed resolution status; the frozen original R3D package is unmodified. Strict ISO C89 GCC and Clang `-Wall -Wextra -Werror -pedantic` builds pass.

## Boundaries and next step

The R3E linker output is **only a link plan**. Zero unresolved symbols does **not** mean all relocation kinds have been implemented or that an executable can be generated. LE address assignment, full segment class/combine handling, precise placement of `_edata` and `_end` within the data object, COMDEF materialization, cross-module FIXUPP32 application, DEF-file import resolution, and LE loader fixups remain to be implemented. The narrow synthetic R3A LE writer is unchanged. The Microsoft hello reference is still 11,806 bytes, not yet reproduced by this workalike.

## R3Q
See `README-R3Q.md`. The new independent raw FIXUPP thread decoder and verification live in `tests/r3q_thread_audit.py` and `tests/verify_r3q.py`.

R3T addition: see `README-R3T.md` and `make verify-r3t` for read-only OMF→Microsoft-LE relocation-site correlation.


R3U addendum: see README-R3U.md and `make verify-r3u`.

## Current experimental milestone: R4D

See `README-R4D.md` and `HANDOFF-R4D.md`. The R4D writer emits a **source-only** 32-bit OS/2 LE from `hi.obj`, `LIBC.LIB`, `OS2386.LIB`, without Microsoft reference-EXE scaffold. Its 171 loader target tuples and complete virtual images match the Microsoft reference after emission. Windows runtime has **not** been tested. R4B and R4C remain frozen baselines.
