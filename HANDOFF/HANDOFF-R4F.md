# LINK386 R4F — SarienPM independent-link experiment

## Baseline preservation
R4D-FIX1 (live-proven hello) and R4E-FIX1 (live-proven CMD32/console) remain unchanged. R4F is a separate fork of R4E-FIX1.

## Inputs and commands
Original `sarienPM-built(1).zip` supplies 42 precompiled OBJ files, `sarien.lnk`, `sarien.def`, `sarien.exe` reference. R4F uses `fixtures/LIBC.LIB`, `fixtures/OS2386.LIB`. The exact command is recorded in `build/command.txt` and run from this directory with `./link386r4e` (recompile using `make link386r4e`). Microsoft `sarien.exe` is NOT opened or used during construction; it is only inspected afterward by verification.

## What was implemented
- R4E writer now permits two-object i386 LE images as well as its original three-object C/386 CMD32 case, and emits physical data pages ahead of code according to the selected profile.
- For a two-object image, image base policy is code `0x10000`, data `0x30000`, consistent with the Microsoft Sarien reference; the code logical extent is rounded to four bytes.
- Sarien effective stack request is 256,000 from `STACKSIZE` in `sarien.def`, taking precedence over the response-file `/STACK:384000`. The invocation explicitly passes `--stack=256000`; generic precedence option parsing is **not** implemented.
- The existing parser/relocator selects 173 modules, places 414 contributions, processes 4,425 OMF fixup records expanded into 4,430 source sites, evaluates 4,348 sites and defers 82 DLL imports with zero other unresolved references.
- The emitted file has 2 objects, 23 physical pages, 4 imported modules, entry object 1+0xF550 and stack object 2+0x5FCB0.

## Explicit limitations — IMPORTANT
THIS IS NOT LIVE-PROVEN OR EVEN BYTE-ORACLE-VERIFIED. Current comparison against original Sarien shows 13,573 code-object byte differences and 842 data-object byte differences. Some are relocations, but substantial semantic/layout discrepancies remain unclassified. Do not freeze this as a correct Sarien linker or assume it will run. PMWIN/PMGPI/QUECALLS imports are discovered from original OMF library records; there is no need for PMWIN.LIB or QUECALLS.LIB. No Windows runtime tests performed.

The Microsoft Sarien reference has a data object page-table length larger than the four physically stored initialized pages; R4F currently emits only the physical initialized-page entries. It matches physical page mapping and logical in-memory object length, but that extra zero-page representation remains to be analyzed for compatibility.

## Suggested R4G
1. Compare contributions and relocation-mask coordinates against the reference object bytes; classify mismatches as absolute relocation vs raw content vs ordering.
2. Investigate default libraries, padding, OMF communal/common ordering and startup hooks; do NOT copy Microsoft output bytes.
3. Decode and compare all LE loader fixup sites/targets, especially PMWIN/PMGPI/QUECALLS ordinals and page-spanning fixups.
4. Implement a two-object zero-page table model if os2host32 requires it.
5. Only then try to claim runnable correctness on Windows.

## Reproduction
`make link386r4e`; run command in `build/command.txt`; `python3 tests/verify_r4f.py`; `make verify-r4e-fix1 verify-r4d-fix1`.

## Independent mapped-byte discrepancy audit (2026-10-17)
`python3 tests/r4f_compare.py` compares source-produced LE logical images against Microsoft's reference after following the physical page map. Current byte differences by source mask classification:

| Object | Unsourced/zero | Ordinary sourced | Relocation-marked |
|---|---:|---:|---:|
| 1 (code) | 64 | 9,649 | 3,860 |
| 2 (data) | 4 | 799 | 39 |

Ordinary code differences first appear at object 1 offset `0xF740` (near the tail), while ordinary data differences begin at object 2 offset `0x2B14`. These are NOT equivalent binaries and should not be promoted to live-proven status without investigation.

## R4F final validation warning
The Microsoft Sarien reference records **66** entries in the data object's
object-table page-map count, despite only four initialized physical data pages.
Our current writer records four; setting a naïve ceiling of virtual bytes/page
would produce **96**, not 66. Thus the page-count encoding needs more research.
No unsupported value was hard-coded into the output.

The candidate continues to differ at **9,649 ordinary code bytes and 799
ordinary data bytes**, plus relocation-marked differences; see
`tests/r4f_compare.py`. The first ordinary code mismatch is at `0xF740`
and first ordinary data mismatch is at `0x2B14`. No Windows runtime
verification or semantic correctness is claimed.
