# LINK386 R4H — Sarien differential relocation audit (2026-10-17)

## Baselines and evidence
- R4D-FIX1 hello and R4E-FIX1 CMD32 remain frozen, live-proven, and unchanged.
- R4F and R4G SarienPM independently generated LE files reach actual AGI script execution but fail with `soft386: guest halted without process exit`.
- The user supplied an earlier soft386 run of `sarienle.exe` reaching `Game ID: "SQ2"` without a termination line in the supplied excerpt. Its entry point `0001F680` is DIFFERENT from the R4G image entry `0001F550`, so it is not treated as byte-identical or necessarily generated from the same build.
- Microsoft `fixtures/sarien/sarien.exe` is the byte-level oracle and is NEVER consulted during executable generation.

## R4H implementation
Added `tests/r4h_compare.py` to attribute all **ordinary-byte mismatches** to source OMF contributions using the R4G placement log, with full byte-by-byte leading-data-cluster dump. Added `tests/r4h_fixups.py` to decode source/target/alias semantics for every LE loader-fixup site and compare Microsoft and independent output, using correct nonsequential LE physical page mapping and repeated source-offset lists. `make verify-r4h-fixups` rebuilds the Sarien R4G candidate first, then runs these two forensic checks. No producer source changed; R4H is an evidence checkpoint, NOT a repaired Sarien linker. R4G and all verified baselines remain intact.

## Verified findings
- R4G object 1: 9,443 ordinary initialized-byte differences; first at 0xF848 (`crt0msg.asm` `_TEXT`).
- R4G object 2: 799 ordinary initialized-byte differences; first at 0x2B18 (`crt0msg.asm` `_DATA`).
- High concentrations of ordinary differences in `output.asm`, `realloc.c`, `heapadd.c`, `malloc.c`, `heapgrow.c` code, and `ctype.asm`, `crt0msg.asm` MSG and `output.asm` data. See `build/r4h-differential.txt` for individual counts and actual byte values.
- Microsoft LE fixups: **1,374 records, 2,789 expanded sites**; R4G: **2,789 records, 2,789 expanded sites**. Exactly **1,423** source/type/target site entries match, **1,366** R4G entries unmatched and **1,366** Microsoft entries absent. This requires rectification BEFORE trusting PM execution. See `build/r4h-fixup-diff.txt`.
- R4G data cluster: `fclose.c` `_DATA` at 0x2B14; `crt0msg.asm` `_DATA` at 0x2B18; `chkstk.asm` `_DATA` at 0x2B1C. This is the first ordinary-data divergence. The code divergence at 0xF848 also belongs to `crt0msg.asm` `_TEXT`. This is correlated placement evidence, not definitive causation of runtime halt.
- The external import set is resolved enough for SarienPM to initialize graphical output, queue references and script execution, but the current failing EIP and guest exception are UNKNOWN. The earlier trace does not identify a DosBeep failure or prove that QUECALLS is the cause.

## Recommended R4I
1. Recover *complete* runtime member selection/extraction order using the Microsoft LE as a post-build oracle, and fix contribution ordering in the linker rather than accumulating Sarien-only pair moves.
2. Rebuild and require fewer mismatches in **both** `r4h-differential.txt` and `r4h-fixup-diff.txt`, including exact PMWIN/PMGPI/QUECALLS import sites and targets.
3. Correct data initialized records / COMDEF ordering as supported by the source; retest true 66-entry LE virtual data page-map semantics (R4G uses four populated entries).
4. Capture soft386 guest halt EIP / instruction / fault status and correlate with the correct reference and R4G contributions; compare sound-thread / queue/DosBeep path only once that evidence exists.
5. Only claim Sarien live success if the actual guest reaches stable gameplay under the independently linked executable.

## Reproduce
```
make -B link386r4e
make verify-r4h-fixups
make verify-r4e-fix1 verify-r4d-fix1
```
