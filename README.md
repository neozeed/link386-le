# LINK386 R4L — Input-derived OS/2 LE linker ordering

**Milestone: R4L build/oracle verified, Windows runtime not separately tested.**
The generated Sarien LE is **byte-for-byte identical** to the live-proven
R4K Sarien executable, but the R4L branch has not itself been executed on
Windows during development.

R4L eliminates the Sarien-specific placement manifest from the *linker
source*. Its default chooses a bounded layout policy using only selected OMF
record metadata:

- **Flat 32-bit CODE:** breadth-first external-symbol dependency levels from
  command-line OBJ modules; stable archive member order within each level;
  apply that order to both CODE and initialized DATA contributions.
- **Communal symbols:** allocation order derived from the historical LINK386
  256-bucket case-insensitive symbol hash (rather than a named symbol table).
- **Mixed 16/32-bit CODE:** use the existing stable module/segment order,
  preserving the live-proven CMD32 bridge behaviour.
- **Output object bases:** derive nonoverlapping 64 KiB-aligned bases from the
  preceding virtual object extent, rather than naming a Sarien base.
- **Local symbols:** LEXTDEF resolves to the defining module's LPUBDEF first.

This is a measured compatibility policy across the three known programs, not
proof of a universally equivalent implementation of Microsoft's archive
searcher for every possible OMF input. Options: `--omf-auto-order` (default),
`--legacy-order`, and `--r4l-bfs-order` (diagnostic override).

## Build

```
make link386r4l
make verify-r4l
make verify-r4e-fix1 verify-r4d-fix1
```

The full/private fixture package contains original Microsoft C/386 OMF
objects, `LIBC.LIB`, `OS2386.LIB`, Microsoft reference executables, and Sarien
application objects for testing. The source-only Git package deliberately
excludes proprietary binary inputs and reference executables.

## Sarien build

```
bash build/r4l-command.sh
```

This builds `build/sarien-r4l.exe` from the 42 application objects, DEF and
both libraries. It does **not** read a reference EXE. Verification subsequently
checks the emitted executable against the Microsoft oracle.

Expected standalone output: 113,756 bytes; SHA-256
`fd845cf0839bdb2fc77740c3e7e05616cd9eb731fd27b96809562ab4a25ca287`.
This is also the exact SHA-256 of the Windows-live-proven R4K Sarien executable.

## Regression evidence

- Sarien: entire mapped CODE and DATA images identical; all 2,789 LE loader
  site/type/target tuples exact; final EXE bytes identical to frozen R4K.
- CMD32 console: 19,554-byte output; 319 fixup sites exact.
- Full CMD32: 91,746-byte output; 1,232 fixup sites exact.
- C/386 hello: R4D-FIX1 independently built output and source-modified `yo!`
  test retain their frozen regression results.
- GCC and Clang compile strict C89; both reproduce identical Sarien output.

R4K remains frozen and untouched. R4L is a **separate** engineering tree.
For details, limitations, and handoff, see `HANDOFF-R4L.md`.
