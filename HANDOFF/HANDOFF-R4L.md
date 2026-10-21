# LINK386 R4L — Hand-off / frozen R4K successor experiment

Date: 2026-10-21

## Frozen baselines

- R4D-FIX1: independent C/386 hi/yo LE linker, live-proven Windows/os2host32.
- R4E-FIX1: independent multi-object CMD32 console + full shell, both
  live-proven Windows/os2host32.
- R4K: SarienPM with graphics, sound, saved-game restoration, live-proven
  Windows/os2host32; its 113,756-byte executable SHA-256 is
  `fd845cf0839bdb2fc77740c3e7e05616cd9eb731fd27b96809562ab4a25ca287`.
  Archive SHA-256 is
  `be49c976c1b1b630478c726afae54de75f015735d598ffaa74cf8f02ae3c0189`.
- NONE of these frozen archives/sources was modified by R4L.

## What is new

1. Deleted the `r4k_order.inc` 104-entry Sarien placement table and old
   `r4j_order.inc` from the R4L C source tree. No Sarien module names/offsets
   are consulted by the new linker.
2. Traverses selected OMF external-symbol dependencies breadth-first,
   starting at command-line OBJs (depth 0). A referenced selected LIB member
   gets the minimum depth of its referencing module + 1. Contributions are
   stable-sorted by `(depth, input/archive index, segment index)`.
3. Uses that order for both CODE and initialized DATA contributions in the
   pure-32-bit case. Applying it only to CODE already gave exactly placed
   code but left 799 ordinary DATA byte mismatches. Applying it to DATA
   eliminated them *all*.
4. Generic c_common ordering follows a historical 256-bucket name hash:
   `(length + sum((lowercase-like byte | 0x20) << (position & 3))) & 255`.
   The source stores buckets in descending order because the inherited
   common-VA calculation walks the allocation table backwards. No symbol
   names are hard-coded into this policy.
5. For mixed 16/32-bit CODE input, conservatively retains the stable
   selection/contribution ordering proven in CMD32. The profile is chosen
   by SEGDEF use16/use32 attributes, not program names or DEF filenames.
6. For 2-object LE, place DATA at the next 64-KiB boundary after CODE's
   actual virtual extent, replacing Sarien's hard-coded 0x30000 base.
7. The linker is built as `link386r4l` (same source also builds the historical
   `link386r4e` alias required by existing regression scripts).

## Verified, reproducible results

Run `make link386r4l verify-r4l verify-r4e-fix1 verify-r4d-fix1`.
`verify-r4l.py` copies just OMF+LIB+DEF inputs to an isolated temporary tree;
no Microsoft prelinked executable is supplied. After building (twice for
repeatability), it reads the Microsoft reference and reports:

- Sarien mapped CODE bytes: 0 differences.
- Sarien mapped DATA bytes: 0 differences.
- Sarien loader fixups: 2,789 exact / 2,789.
- Sarien generated EXE: 113,756 bytes and exact SHA-256 matching frozen R4K.
- Compiler: strict C89 GCC and Clang produce byte-identical Sarien EXE.
- CMD32 console/full original C89 regression output: PASS, including 319/1,232
  relocation sites respectively.
- R4D-FIX1 hello/yo original C89 regression output: PASS.

## What is not proven

- R4L output was **not independently live-run under Windows** this turn;
  its generated EXE is byte-identical to R4K, which was live-proven.
- The SEGDEF-based mixed/flat policy is an empirical interoperability
  heuristic; arbitrary OMF applications may require alternate link ordering.
- CLI is still a bounded C/386-profile linker with `-o`, `-d`, explicit OBJ and
  LIB inputs. It is not complete LINK386 command/response/DEF/MAP parity.
- Generic sorting is currently opt-out with `--legacy-order`; further
  variants of 16/32-bit mixed segment layouts require more fixtures.
- Original linker emits more compact fixup records; R4L emits equivalent
  individual site records, so the generated EXE need not be byte-identical to
  Microsoft's reference despite byte-identical mapped objects.
- Proprietary MS runtime libraries, original objects, and Microsoft linked
  reference executables should be excluded from a public Git repository unless
  the distributor has rights. This package contains them for private testing.

## Next milestones

- Add linker-native `.LNK` response file grammar and classic LINK386 prompts.
- Derive the archive/depth order from full OMF resolution semantics across
  multiple libraries and test applications with different module orders.
- Remove remaining profile-specific entry/stack/link-mode heuristics using DEF
  `NAME/WINDOWAPI`, `STACKSIZE`, link option precedence, and OMF class records.
- Add MAP output and diagnostics; cover weak externals, 16-bit and overlay
  edge cases using independent fixtures.
- Port release build tests to MinGW (Win32 and Win64) and verify native OS/2
  builds separately; Linux GCC/Clang builds are proven here.

## Source repository preparation

The separate `LINK386-R4L-GIT-SOURCE.zip` intentionally contains neither
Microsoft binaries nor proprietary application object files. Import your
own legally distributable fixtures for tests. Run `git init`, commit the
source, and tag R4L after reviewing licenses/redistribution rights.
