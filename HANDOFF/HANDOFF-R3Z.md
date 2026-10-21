# LINK386 workalike — R3Z full engineering handoff

Date: 2026-10-17. Baseline: R3Y frozen; R3Z is a separate, additive read-only forensic milestone.

## Goal
Create a native portable C89 Microsoft C/386 OMF-to-OS/2 LE workalike. No DOS, OS/2, Wine or old LINK386 runtime dependency. Three historical acceptance targets: `hi.obj + LIBC.LIB + OS2386.LIB -> hi.exe`, CMD32 (multiple OMF modules, DEF, mixed 16/32-bit code), and SarienPM (42 objects, communal globals, graphical API imports).

## Current executable status — critical
No complete Microsoft-input LE link has been produced by this project. The `link386r3` executable writer currently handles only bounded synthetic cases; the R3G/R3N/R3O programs are planners/diagnostics. DO NOT describe candidate addresses, relocation counts, or a successful verification of a reference binary as actual executable linking. In particular, no approved real FIXUPP32 writeback path yet.

## Source tree
`src/link386r1.c` OMF reader; `src/link386r2.c` OMF link planner; `src/link386r3.c` small synthetic LE writer; `src/link386r3f.c` and `src/link386r3f_core.inc` layout logic; `src/link386r3g.c` segment/group-aware planner; `src/link386r3n.c` candidate fixup classifier; `src/link386r3o.c` blocker diagnostics. `tests/` includes all independent verification/audit scripts from prior milestones, R3Z adds `tests/r3z_prefix.py`. `fixtures/` includes original historical user inputs. `oracles/` includes Microsoft LE/MAP reference artifacts and Ghidra-derived pseudocode. Original complete byte inventory is in `MANIFEST-SHA256.txt`.

## Project chronology / proven boundaries
R1-R2 OMF records, symbols, libraries; R3A synthetic LE emission; R3C recursive LIDATA32, runtime archive intake; R3D COMDEF; R3E linker boundary symbol IDs; R3F-R3M evolving group/segment layout; R3N classification not writeback; R3O-R3R forensic blockers; R3S real LE loader-fixup oracle (78 records, 159 internal sites + 12 imports); R3T zero exact matching LE sites from the provisional planner; R3U predicted import sources all placed in code, Microsoft reference has 8 code and 4 data; R3V proven eight code import sites match provisional addresses after +0x1010 translation; R3W byte matches for heap code in LE object 2; R3X proves GROUP absence and _TEXT attributes alone do not distinguish heap contributions; R3Y 22 strong fingerprint-based _TEXT anchors with ambiguity retained; R3Z mapped prefix evidence.

## R3Z verified finding
Reference `hi-reference.exe` (11,806 bytes) has LE page data starting at the offset in LE header +0x80. Object1 base=0x10000, 2 mapped 4096-byte pages, object2 base=0x20000. The first page 0x10000-0x10fff contains runtime strings (e.g. `MS Run-Time Library`) and `hi!\n`; it is NOT an empty 4 KiB padding region. The second page includes startup instruction bytes and the independently fingerprinted crt0.asm contribution at 0x11020. Therefore the difference 0x1020 cannot be explained as solely a blank prefix. This does NOT yet prove a unique placement algorithm, and some byte matches are ambiguous/overlap.

## Remaining blockers before a real LE linker
1. Reconstruct exact allocation and output ordering across mixed CODE and DATA classes, with contribution identity not inferred only from segment names; correctly honor source-initialized literals occupying first page and _TEXT contributions in the second LE object.
2. Resolve source fixup coordinate and external-index ambiguities without guessing: crt0.asm LIDATA coordinate 13 after 4x4-byte chunks, heapgrow.c ext index 13 vs 12 enumerated externals, chkstk.asm `_end` with addend 0x400.
3. Implement fully checked cross-module OMF FIXUPP32 frame/target method semantics, span validations and actual writeback, keeping supported forms whitelisted.
4. Implement LE loader internal/import fixup generation, verify 159 internal and 12 import sites against reference, then determine entry and stack accurately.
5. CMD32: preserve 98-byte USE16 code contribution, explicit 61440-byte STACK, a separate `c_common`; 16-byte `_TEXT` discrepancy remains. SarienPM: 42 modules and 4 runtime import DLL names, PMWIN/QUECALLS do NOT require on-disk `.LIB` archives; DEF imports matter.
6. Ensure all selected linker library modules and source object contributions are inventoried in deterministic order; reference compiler/ABI uses old OMF class and group concepts even for flat 32-bit outputs.

## Exact commands
`make all link386r3g link386r3n link386r3o` compiles strict-C89 GCC tools.
`make verify-r3z` runs R3Y and R3Z evidence tests (expected `R3Z PASS`).
`make verify-r3s verify-r3t verify-r3u verify-r3v verify-r3w verify-r3x` exercises output-site and placement evidence.
The full inherited `make verify ...` suite can be computationally expensive; capture the log and report exactly which targets finished. Do not claim completion of a timed-out suite.

## Suggested next milestone R4A
Build a complete contribution ledger indexed by original module, library member, segment index/name/class/attributes, LEDATA/LIDATA expanded offsets, and OMF source FIXUPP sites. Join it against strong LE byte anchors without forcing unidentified contributions into arbitrary output objects. The ledger must preserve ambiguous candidates, allow object2 code, and produce enough evidence to revisit the LE writer.

## Reference integrity
Original R3Y untouched. R3Z files are a full self-contained source-and-fixture copy; see SHA-256 manifest. Reproducible reports are evidence, not linking claims. No external tools or web resources needed to inspect this package.
