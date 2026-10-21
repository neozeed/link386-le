# R4G preliminary differential findings — 2026-10-17

This analysis compares `build/sarien-r4f.exe` with Microsoft's `fixtures/sarien/sarien.exe` by reconstructing virtual objects through each LE's *physical page map*. The R4F output is not yet equivalent and no production linker fix was made.

## First hard discrepancy — runtime contribution order

The two code objects agree in ordinary unrelocated bytes until object 1 offset `0xF740`. In R4F's placement log (`build/sarien-intake.log`), this offset starts `crt0msg.asm` `_TEXT`, size 36. Microsoft instead has the prologue `55 8B EC 83 EC 1C 57 56 BF FF FF FF FF 8B 75 08...`. This **exact 24-byte sequence** appears at object 1 offset `0xFA98` in the R4F image, where its placement log identifies `fclose.c` `_TEXT` (188 bytes). Thus Microsoft's link placed at least `fclose.c` earlier than our library-selection-order pass. Re-sorting only its relocation targets cannot repair this.

## Second hard discrepancy — related CRT data ordering

The first ordinary data-object difference begins at offset `0x2B14`. R4F's placement log puts `crt0msg.asm` `_DATA` at `0x2B14` (four bytes); Microsoft's object instead has `5C 00 5C 00` at that offset, where R4F holds `00 00 00 00`. R4F's code and data differences both begin at the same library member's contribution, strengthening the hypothesis that selected CRT archive members differ in ordering and/or inclusion. These facts do **not** prove `crt0msg.asm` must be omitted altogether.

## Quantitative oracle (R4F baseline)

| Object | First ordinary mismatch | Ordinary bytes different | Relocation-marked different | Unsourced different |
| --- | --- | ---: | ---: | ---: |
| 1 code | `0xF740` | 9,649 | 3,860 | 64 |
| 2 data/stack | `0x2B14` | 799 | 39 | 4 |

The reference LE itself contains 107,612 bytes; the R4F candidate 113,756. The code-object length is 74,844 and data/stack extent 392,368 in each, but this matching extent is not sufficient to establish correct contents or fixup targets.

## Recommended next repair experiment

1. Recover Microsoft's exact CRT library-member extraction/order from code fingerprints and OMF symbol dependencies; use `fclose.c` at reference code `0xF740` as first fixed anchor.
2. Rebuild the *unrelocated* code and data after changing **general** archive-member ordering. Check that both mismatch boundaries advance, and compare all initialized bytes, not only relocated sites.
3. Recompute public symbol addresses and every loader import target after layout changes. Independently compare all LE fixup source and target pairs to Microsoft's, including PMWIN/PMGPI/QUECALLS.
4. Only after byte and fixup-oracle agreement re-test Sarien in soft386; request guest EIP/fault type if it continues to halt.

Do not patch the output by copying reference bytes. Preserve R4E-FIX1 and R4F unchanged.
