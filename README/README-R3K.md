# LINK386 R3K — reference-guided text-tail provenance diagnostic

Status: BUILD/REGRESSION VERIFIED; NO NEW EXECUTABLE LINK.

R3K follows R3J. It adds `tests/r3k_reconcile.py`, a reproducible
Microsoft MAP versus R3G link-plan comparison. It preserves the R3J
planner and existing LE writer unchanged.

## Findings

For the seven CMD32 objects plus real LIBC.LIB and OS2386.LIB:

- `_TEXT` planned size 61,889 (0xF1C1), Microsoft size 61,905
  (0xF1D1), delta +16 bytes.
- Last selected `_TEXT` contribution: `\\mrt386\\common\\x8tmul.asm`,
  local offset 0xF180, size 65 (end 0xF1C1).
- The missing 16 bytes are not attributed to a specific record or linker
  action. They MAY relate to generated material or an unmodelled contribution;
  neither explanation is proven. No speculative bytes were inserted.
- OMF16 CODE16 and STACK/c_common comparison still match Microsoft's map.
- Some MAP segment names have multiple candidate R3G Outseg entries because
  group/combine policies differ; R3K labels these UNMATCHED rather than
  choosing a possibly wrong one.

## Reproduce

```
make link386r3g
make verify-r3k
make verify-r3j verify-r3i verify-r3g
```

Inspect `r3k-reconciliation.txt` for individual sizes and ambiguities.

## Next engineering step

To attribute the 16 bytes, trace all OMF segment contributions, MODEND
entry and linker-generated thunk/import-related bytes, then establish the
full GROUP-aware segment identity. Do not pad until the origin is proven.
R3K does not implement cross-module fixups or genuine LE emission.
