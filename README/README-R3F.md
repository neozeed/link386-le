# LINK386 R3F — deterministic contribution-layout prototype

R3F extends the frozen R3E OMF model by retaining SEGDEF class names, and adds `link386r3f`, a new standalone ISO C89 **layout report** command. No LE executable is emitted. R3E files remain available in their original package.

## Building and running

```sh
make all
make verify
./link386r3f fixtures/hi.obj fixtures/LIBC.LIB fixtures/OS2386.LIB > hello-layout.txt
./link386r3f fixtures/sarien/*.obj fixtures/LIBC.LIB fixtures/OS2386.LIB > sarien-layout.txt
```

The planner selects archive modules transitively through the existing R3E symbol resolver. It reports CODE, initialized DATA and uninitialized BSS/STACK contributions, offsets, recorded alignment, classes and combine flags; resolves PUBDEF addresses relative to their region; and assigns aligned offsets to coalesced COMDEF symbols after BSS/STACK contributions. Strong publics override nonlocal communal allocations. Bounds and overflow checks constrain storage.

**Important limitations:** This is a deterministic *provisional* region ordering, **not** Microsoft LINK386's correct final layout. Identically named segments are not yet coalesced under their OMF combine semantics, group positioning is incomplete, address calculation uses simple class grouping and input/member order, and `use32=0` segments are merely described. `LIDATA` bytes are parsed by the underlying R3E intake but no linked output is written. `_edata` and `_end` are shown as REGION END CANDIDATES only (not exact symbol offsets or linear VAs). R3F does not produce executable bytes or apply cross-module fixups. In particular, its offsets must not be treated as authoritative for either hello or Sarien. Actual Microsoft reference binaries should continue to be used as layout oracles.

## Tests

- GCC and Clang compile the new tool with `-std=c89 -pedantic -Wall -Wextra -Werror` and emit byte-identical hello reports.
- Existing R2–R3E `make verify` suite passes.
- Hello: 120 contribution records, 2 coalesced commons.
- SarienPM: 414 contribution records, 34 coalesced commons.

R3G prerequisite: implement OMF combine semantics and accurate segments/groups/stack alignment, then validate against Microsoft MAP/LE outputs, before relocation patching or LE emission.
