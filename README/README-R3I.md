# LINK386 R3I — OMF use16/use32, stack override, c_common output segment

R3I builds on R3H. It fixes the historical mixed-width error uncovered by the Microsoft CMD32 MAP oracle and extends the conservative planner. It is **not** an executable-linking milestone.

## Corrections

* For SEGDEF (0x98) and SEGDEF32 (0x99), the record variant determines the width of the **segment length field**, not segment execution bitness. The ACBP attribute's low P bit determines `use32`. This corrects the real `$C20006_CODE16` 98-byte segment to `use32=0`.
* `link386r3g --stack=61440 ...` raises the output STACK combined segment to the specified minimum size. This is an explicit CLI override, **not** yet full /STACK or DEF parsing; it preserves existing contribution offsets.
* `c_common` now exists as a named BSS output segment rather than an anonymous extension. A global/common allocation still follows the historical R3D/R3E preliminary plan; ordering and grouping have not been proven to match Microsoft.
* `verify_r3g.py` has its expected combined segment count updated to include `c_common`. All existing OMF and synthetic LE writer regressions remain in place.

## Reproduce

```
make all link386r3g
make verify verify-r3g verify-r3h verify-r3i
clang -O2 -std=c89 -pedantic -Wall -Wextra -Werror -o link386r3g-clang src/link386r3g.c
```

R3I regression compares these Microsoft CMD32 MAP fields against the actual planner output: `$C20006_CODE16` CODE use16 98 bytes; STACK 61440 bytes (with --stack); `c_common` BSS 704 bytes; `_BSS` aggregate 259676 bytes.

## Limitations / next task

* The output remains a **region-relative segment plan**; entry-point determination and group frame synthesis are incomplete.
* The exact `_TEXT` and `_DATA` layout discrepancies (16 and 3 bytes respectively) remain; investigate padding, alignment and segment ordering before applying relocations.
* A `c_common` segment is synthesized, but the final symbol offsets and LE data-object representation are **not proven**.
* No final import table, relocation fixup table, DEF processing or runnable hi/CMD32/Sarien LE output is produced.
* PMWIN and QUECALLS are runtime DLL names, not missing static link libraries.

This is a compile-and-fixture-verified milestone only.
