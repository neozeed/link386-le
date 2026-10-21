# LINK386 R3S — Independent Microsoft LE fixup oracle

R3R remains frozen. This independent R3S tree adds a **read-only** decoder of the actual LE fixup streams from Microsoft's 11,806-byte `hi.exe`. Run `make verify-r3s` to reproduce `r3s-le-fixup-oracle.txt`.

## Established by exact bytes

* Two LE objects at 0x10000 and 0x20000, with three logical pages in the page table; the only import module is `DOSCALLS`.
* Three fixup-page spans contain **78 target records covering 171 relocation sites**, because some records use a list of signed page-relative source offsets.
* Of those 171 sites, **159 are internal** and **12 import by ordinal from DOSCALLS**. The independently produced counts agree with the earlier WHP/ReactOS LE loader oracle.
* Import source kind 8 identifies 32-bit self-relative source relocations in the reference.
* The decoder aborts on unsupported target kinds, unsupported target flags and malformed record crossings, rather than guessing their meanings.

## Not yet proven

This run does **not** associate the three R3R ambiguous OMF fixups (`crt0.asm`, `chkstk.asm`, `heapgrow.c`) with exact LE site addresses. That requires audited contribution-to-LE placement, source attribution and effective fixup semantics. In particular, an OMF FIXUPP32 count is not interchangeable with an LE loader-fixup site count: intra-object PC-relative references can be fully resolved at link time and disappear from LE loader fixups.

No linker output, file mutation, executable runtime or relocation writebacks are claimed. The prior C89 linker is preserved unchanged. Subsequent work should use these 171 exact LE reference sites as a check when implementing real LE fixup emission.
