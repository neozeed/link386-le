# LINK386 R3X — historical SEGDEF/GRPDEF versus Microsoft LE object ownership

Frozen baseline: R3W unchanged. This is a separately packaged **read-only provenance and acceptance-test milestone**; the C linker is unchanged and still does not link `hi.exe`.

## Confirmed directly from LIBC.LIB

`heapgrow.c` and `heapinit.c` declare a 32-bit `_TEXT` segment in class `CODE` with OMF combine=2, align=5, but no actual group membership. Their `GRPDEF FLAT` contains **zero member entries**. They have 32-bit segment-length records, with actual 32-bit use indicated by SEGDEF attribute bit 0.

**Critical negative control:** `crt0.asm`, `crt0dat.asm`, `lseek.asm`, `write.asm`, and `nmsghdr.asm` also have `_TEXT` without group membership, even though their `_TEXT` bytes occur in LE object 1. Therefore *absence of a group alone cannot explain the object-2 placement*. A special-case rule saying ungrouped text belongs to object 2 would be provably wrong.

The real hello LE object flags are `0x2005` (object 1) and `0x2003` (object 2), confirming that LE object 2 has different attributes despite including code bytes. The object-2 `_TEXT` byte fingerprints at `0x202A0` and `0x204E4` are verified by the previous independent R3W fixture, but not all fixup source offsets are attributed yet. The apparent +0x1010 shift in object-1 import sites remains unexplained.

## Reproduce

`make verify-r3x` validates the SEGDEF32/GRPDEF records directly from the bundled `LIBC.LIB`, checks both LE object flags, and reruns R3W's fingerprint/ownership regression. For inherited C89 tests use `make link386r3g verify-r3j verify-r3l verify-r3m verify-r3n`. Output: `r3x-segment-provenance.txt`.

## Next investigation

Inspect OMF *combine, alignment, class and ordering* across all 39 selected hello modules and locate precise segment-contribution offsets relative to the two LE objects. Verify OMF source fixup records against four object-2 import sites; do not invent a group rule or apply a uniform shift. Runtime API imports may be represented through existing OS2386.LIB members; no separate PMWIN or QUECALLS import libraries are assumed.
