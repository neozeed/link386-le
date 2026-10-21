# LINK386 R3J — class-ordered layout, verified against Microsoft CMD32

Status: BUILD AND STATIC FIXTURE TESTS PASS; no full LE link.

## Implementation

`src/link386r3g.c` now builds a stable layout order by segment region and
class, without modifying the existing selected-module list, original
contribution-relative offsets, segment sizes or OMF combine attributes.

- CODE use32 before CODE use16 (still separate 16/32 segments).
- BEGDATA before DATA, followed by CONST and MSG.
- BSS before c_common, then STACK.
- Distinct segments and different GROUP memberships remain distinct.

This is a *CMD32-evidenced ordering subset*, not a universal Microsoft linker
class table; first-seen order is preserved within each class group. The
physical LE writer is NOT hooked to this planner yet.

## New Microsoft MAP oracle

`tests/verify_r3j.py` checks the actual CMD32 reference map against the
planner with `--stack=61440` and both original libraries:

- NULL 0x0000, _DATA 0x0044, XIB 0x2f1c
- CONST 0x2f34, HDR, MSG, PAD and EPAD all at exact MAP offsets
- c_common before STACK, with the correct individual allocated sizes
- The two existing `_DATA` output contributors total 11,989 bytes, but
  span 11,992 bytes including an observed 3-byte internal alignment gap.
  That explains the prior apparent three-byte discrepancy **without**
  adding data or fabricating content.
- `_TEXT` is still 61,889 bytes in the current planner versus Microsoft's
  61,905 bytes; the remaining 16-byte discrepancy has NOT been explained
  and no speculative padding has been added.

## Verify

```
make all link386r3g
make verify verify-r3g verify-r3h verify-r3i verify-r3j
CC=clang make -B link386r3g
make verify-r3j
```

The next milestone should trace the final 16 code bytes to a specific
input contribution or output-generating linker action, then implement group-
aware final addresses and cross-module FIXUPP32 relocation with real imports.
R3J does not generate or execute a new `hi.exe`.
