# LINK386 R4I — Sarien runtime ordering oracle

## Immutable baselines
R4D-FIX1 hello and R4E-FIX1 CMD32 are frozen and live-proven. R4F and R4G source-only Sarien candidates reach AGI script execution but halt without process exit. R4H was a diagnostic fixup/data comparison. This R4I tree is a separate descendant of R4H and does not change the executable producer or claim Sarien gameplay success.

## New independent analysis
`tests/r4i_order.py` maps OMF contributor intervals published by the source-only R4G link against Microsoft's post-build LE reference through multiple 16-byte relocation-masked windows. It reconstructs the *virtual* LE image using the nonsequential page map, then ranks possible positions for each contribution with independently matching windows. It requires at least four accepted windows and three agreeing placements, with a leading vote count at least twice the runner-up. Reference data is consulted ONLY after independent source-only build; neither this tool nor its results feed the executable producer.

This yields 95 strong placements from 121 examined segments: 66 code and 29 data. Other contributions are not safely resolved by this method. This is a fingerprint inference, not proof of LINK386's extraction-order algorithm; ambiguous sequences and relocation-dependent windows remain omitted. The full ordered report is `build/r4i-order.txt`.

Some particularly diagnostic placements (object-relative hex offsets):
- `fclose.c` `_TEXT`: F740 (R4G already correct)
- `fread.asm` `_TEXT`: F888, rather than R4G FBE0 (856 bytes earlier)
- `fwrite.asm` `_TEXT`: F95C, rather than R4G FCB4 (856 earlier)
- `malloc.c` `_TEXT`: FB40, rather than R4G 11440
- `realloc.c` `_TEXT`: FD78, rather than R4G 11690
- `stdargv.asm` `_TEXT`: 107FC, rather than R4G F894
- `output.asm` `_TEXT`: 10EB4, rather than R4G 1029C
- `output.asm` `_DATA`: 2EE0, rather than R4G 2DB8

These show block reordering rather than a single constant-offset correction. The data order is also not identical to the code order. Do NOT simply reorder module selection once for all segments without testing code and data separately.

## Remaining blockers
R4H found 9,443 ordinary code-byte differences and 799 data-byte differences in R4G; 1,423/2,789 relocation sites/targets match and 1,366 do not. R4I's forensic study does not reduce those counts because no producer modification was made. The Sarien halted guest EIP / exception remains unknown. The earlier `sarienle.exe` trace entry F680 differs from R4G F550. The supposed DosBeep/queue/thread role remains a hypothesis, not established crash causation.

## Reproduce
```
make verify-r4i
make verify-r4e-fix1 verify-r4d-fix1
python3 tests/r4i_order.py > build/r4i-order.txt
```
`verify-r4i` operates on the included source-only R4G executable and ledger; use the original build command in `build/r4g-command.sh` to regenerate them as necessary.

## Next recommended iteration
Recover positions of the remaining 26 unresolved contributions using OMF source records and carefully validated fixup-masked longer signatures; deduce a general extraction/traversal rule. Evaluate any order change with R4H's two metrics (initialized bytes AND full fixup site/target sets), not only the first mismatch. Build a new standalone Sarien experiment only once both materially improve. Retain an unmodified R4H/R4I baseline and request guest EIP/fault instrumentation if practical.
