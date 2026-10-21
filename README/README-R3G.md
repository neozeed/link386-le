# LINK386 R3G — OMF segment combination (build-verified; NOT an LE linker)

R3G adds `src/link386r3g.c`, a new portable C89 link-plan command on top of the previously verified R3F parser and member-selection engine. R3F is preserved, and the R2–R3E tests remain unchanged.

## Build and test

```
make all
make verify
make verify-r3g
./link386r3g fixtures/hi.obj fixtures/LIBC.LIB fixtures/OS2386.LIB > hello-r3g.txt
./link386r3g fixtures/sarien/*.obj fixtures/LIBC.LIB fixtures/OS2386.LIB > sarien-r3g.txt
```

## Implemented

* Combine compatible selected OMF contributions keyed by name, class, use32 mode, OMF combine value, region, and explicit group name.
* Preserve private segments individually; append compatible PUBLIC/STACK combine contributions with their own OMF alignment. Tentatively overlay same-name COMMON combine contributions.
* Emit combined segment table, per-module contribution placements, public symbol offsets, communal BSS placements, and region-relative data/BSS boundary candidates.
* Perform overflow checking and use only the archive members selected through the R3E resolver.

**This is a bounded layout experiment, not a Microsoft-equivalent layout implementation.** Actual OMF class-based ordering and group placement, absolute/frame alignment, strict combine-mode semantics, intersegment padding, DOSCALLS fixups, local symbols, final linker boundaries, stack selection, and LE emission remain unimplemented. In particular, `COMMON` combination and merge compatibility are conservative approximations and must be validated against Microsoft's exact MAP/LE oracles before use in executable generation. A successful link is NOT claimed. R3G never writes an EXE.

## Observed regression data

| Real input | Contributions | Combined output segments | Communals | Code region | Initialized region | BSS / stacks including commons |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hello + LIBC + OS2386 | 120 | 26 | 2 | 6158 | 1356 | 8896 |
| 42 SarienPM objects + LIBC + OS2386 | 414 | 26 | 34 | 74826 | 12721 | 131840 |

These are **region-relative measurements**, not Microsoft LE object sizes. GCC and Clang strict-C89 builds produce identical hello layout reports. Prior R2–R3E regression suite passes.

## Next engineering gate

Compare to source-compatible Microsoft MAP/LE references, complete GROUP and combine rules, then build a real relocation evaluator and LE import/fixup tables. The present synthetic LE writer remains a separate R3A prototype.
