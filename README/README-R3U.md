# LINK386 R3U — LE Import Source Ownership Audit

Baseline: **R3T frozen**, with R3U as a separate source tree. No C linker changes and no relocation writebacks.

## Discovery

The original 11,806-byte Microsoft `hi.exe` has 12 DOSCALLS loader import source sites: **8 in LE object 1 (code), 4 in LE object 2 (data)**. All 12 candidate import fixup source sites in the R3N planner are currently assigned to **LE object 1**. The 12 candidate coordinates have **zero exact matches** with the Microsoft output.

This is an object-ownership discrepancy, not merely a single fixed base offset. All sites and associated module names/ordinals are listed by `make verify-r3u` and in `r3u-import-anchors.txt`.

Do not equate an import relocation *source* with an import veneer or resolved import destination. The LE loader fixup is applied at the source site.

## Acceptance gate for the next implementation

1. Recover original module/segment contribution ownership for all 12 import references, including the four that Microsoft places in the data object.
2. Reconcile code contribution ordering and any importer-specific fixup-site offsets, ideally by matching intact instruction/data bytes to the reference LE pages.
3. Only after the 12 independently verified import-source coordinates match should the LE writer enable imported relocation emission.
4. Preserve separate diagnostics for crt0.asm, chkstk.asm, and heapgrow.c.

## Verification

`make verify-r3u`; inherited `make verify-r3t` and `make verify-r3s`. R3U is analysis-only.
