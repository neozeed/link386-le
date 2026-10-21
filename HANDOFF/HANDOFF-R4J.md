# LINK386 R4J — Sarien reference-guided placement experiment

## Scope and baselines
Frozen R4D-FIX1 (live-proven hello) and R4E-FIX1 (live-proven CMD32) remain unchanged. R4I is an evidence-only baseline; R4J is a separate derived tree. This experiment does not resolve the Sarien runtime crash and is **not** a general-purpose linker-ordering rule.

## What changed
- `src/r4j_order.inc` is an explicit Sarien-specific *reference-derived* placement priority manifest built from the R4I post-build, relocation-masked OMF/LE fingerprint report. R4I's Microsoft reference analysis is not embedded as executable bytes, but the placement manifest is derived from that reference, so builds using it are **reference-assisted ordering experiments**, not fully source-only reconstruction of the linker algorithm.
- `src/link386r3f_core.inc` adds a second placement mode for selected library CODE contributions, preserving command-line application object order. The original R4G placement remains the default.
- `src/link386r4e.c` adds opt-in `--r4j-oracle-order`; without that flag R4G/R4E behaviour is preserved. The flag is restricted to a `sarien.def` profile.
- `tests/r4j_constraints.py` compares post-build relocations, masked source bytes, and contribution anchors against the Microsoft reference for diagnostics only.

## Reproduce
```
make link386r4e
bash build/r4g-command.sh > build/r4g-build.log 2>&1
bash build/r4j-command.sh > build/r4j-build.log 2>&1
python3 tests/r4j_constraints.py > build/r4j-constraints.txt
make verify-r4e-fix1 verify-r4d-fix1
```

## Independent comparison results
R4G: 9,443 ordinary code differences, 799 ordinary initialized-data differences; 1,423 / 2,789 exact loader fixup site/type/target matches.
R4J experimental: 9,344 ordinary code differences, 799 ordinary initialized-data differences; 1,421 / 2,789 exact loader fixup matches. Strong R4I anchors exact in R4J: 28/66 CODE, 26/29 DATA.

A naive sort of all contributions moved application contributions and was strongly worse (598/2789 exact fixup matches); this attempt was rejected. The packaged candidate restricts the experiment to library CODE. Although the ordinary code byte metric modestly improves by 99, the fixup oracle slightly regresses (two fewer matching sites). This is NOT a successful overall ordering correction. Do not replace the working Microsoft-linked Sarien or assume the experimental R4J executable is safe.

## Diagnosis and next work
The R4I anchor set alone cannot determine correct ordering for the 26 ambiguous/small contributions, and class-specific DATA ordering still differs. Recover full member-selection and contribution ordering from the original LINK386 implementation/OMF ordering semantics, or use a constrained gap solver that incorporates sizes, alignment, and zero-extent contributions. Any accepted correction MUST improve both masked ordinary source bytes and the complete fixup site/target multiset, and preserve frozen R4D/R4E regression tests. The soft386 halt's guest EIP is still missing; DosBeep/queue involvement is unproven.

## Status
R4J is a fully packaged, tested **negative-result/constraint-audit** milestone, not a new verified SarienPM linker or runtime fix.
