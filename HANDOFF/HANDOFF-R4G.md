# LINK386 R4G — SarienPM ordering-correction experiment

## Frozen baselines
- R4D-FIX1: live-proven independent hello LE.
- R4E-FIX1: live-proven independent CMD32 and console LE.
- R4F: SarienPM source-only candidate; window initialization occurs on Windows, followed by soft386 guest-halt failure. No demonstrated clean exit.
All three are unchanged. R4G is a new independent source tree.

## User's dynamic clue
User suspects failure at DosBeep in a separate message-queue listener thread. The reference Sarien executable contains `\\QUEUES\\SARIEN.QUE`, `DosCreateQueue %s returned error`, and `Error with thread openeing Queue %s` text. This is relevant to investigating runtime behavior but does **not** prove the crash is caused by DosBeep, queue handling, or linker fixups. A faulting guest EIP, status, and a comparison with the Microsoft executable under identical input would be needed. Do not remove this concern from follow-up investigations.

## R4G experimental implementation
- In `src/link386r3f_core.inc`, `place_segments()` uses a **Sarien-profile-only** local ordering correction, materializing `fclose.c` and `fopen.c` just after `crt0dat.asm` rather than after other startup and library components.
- Profile gating is currently by `sarien.def` pathname in `src/link386r4e.c`; this is intentionally NOT a general-purpose linker ordering rule. It is an experimentally oracle-derived constraint. Make this detection robust and derive a general rule from symbol dependencies before promotion.
- All other R4F LE writer behavior is inherited. The Microsoft reference EXE is never read during R4G output generation, only during `tests/verify_r4g.py` comparison.
- An attempted broader dependency-selection-order reordering was rejected because it damaged code placement, although data mismatches improved somewhat. An attempt to move just `crt0msg.asm` was likewise rejected; both original tests remain preserved only as notes, not applied to current R4G.

## Verified comparison
With Microsoft `sarien.exe` as the POST-BUILD byte oracle and source/relocation masks:

| Metric | Frozen R4F | R4G experiment |
|---|---:|---:|
| First ordinary code difference | 0xF740 | 0xF848 |
| Ordinary code differences | 9649 | 9443 |
| First ordinary data difference | 0x2B14 | 0x2B18 |
| Ordinary data differences | 799 | 799 |

The runtime contributions `crt0.asm` 0xF550, `crt0dat.asm` 0xF628, `fclose.c` 0xF740, `fopen.c` 0xF7FC have the expected starting offsets under this experiment. It does not yet resolve all bytes, import fixups or the soft386 guest halt. The original R4F executable remains included for comparison; R4G executable is separately generated.

## Reproduction
```
make -B link386r4e
bash build/r4g-command.sh >build/r4g-build.log 2>&1
python3 tests/verify_r4g.py
make verify-r4e-fix1 verify-r4d-fix1
```
The original `build/command.txt` still names the R4F image, which this workflow does not overwrite. R4G candidate path: `build/sarien-r4g.exe`. Test this as an EXPERIMENT, not as a repaired production Sarien binary.

## Next work
1. Recover extraction-order semantics from the original LINK386/Ghidra and original static input: a symbol-dependency ordering rule that also preserves the live-proven CMD32 order. No hard-coded `sarien.def` test long term.
2. Classify the new difference at code 0xF848 and data 0x2B18 against source-member contribution bytes and fixup records.
3. Complete objective loader fixup site/target comparisons for PMWIN/PMGPI/QUECALLS and reconcile the 66 vs 4 page-map entries.
4. Obtain soft386 fault EIP/reason and inspect DosBeep/queue listener dispatch in the original SarienPM and generated binary, comparing identical scenarios.
5. Do not claim runtime success or freeze R4G as a live-proven linker.
