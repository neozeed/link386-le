# LINK386 R3H — Microsoft LE/MAP reference oracle (verified)

R3H is a **reference-driven diagnostic milestone** built from R3G without changing R3G's placement algorithm or previous link-planner behaviour. It is not a runnable LE linker.

## Build and tests

    make all
    make verify
    make verify-r3g
    make verify-r3h

The R3H target runs `tests/r3h_oracle.py`, an independent standard-library-only parser of MZ/LE object tables, and Microsoft MAP segment/entry-point data. It checks that each MAP segment extent fits its LE object, and that MAP and LE entry points agree. Original references are in `oracles/`; `fixtures/hi-reference.exe` comes from the prior frozen stages. It covers: hi, CMD32, console test, SarienPM. Both available CMD32 MAPs pass. The original SarienPM package has no MAP; its LE is still audited.

`tests/compare_r3g.py` runs the actual R3G combined layout on the seven CMD32 objects and original two libraries; it compares output segment aggregate **sizes** with the Microsoft's CMD32 MAP by `(name, class, bitness)`. The result is `r3h-cmd32-diff.txt`. Differences do not imply matching contribution placement for equal-size segments.

## Confirmed gaps, not silently fixed

* CMD32 `_TEXT` 61,889 vs Microsoft's 61,905: 16 byte difference.
* CMD32 `_DATA` 11,989 vs Microsoft's 11,992: 3 byte difference.
* CMD32 `_BSS` 259,676 exactly matches.
* Microsoft's separate 16-bit `$C20006_CODE16` 98-byte object is miscategorised as use32 by the R3G OMF extraction; this must be corrected before emitting executable code.
* R3G's `STACK` 8,192 bytes differs from CMD32 reference 61,440 bytes, because DEF / command-line stack allocation is not yet propagated into the layout. The MAP places it at 0003:00042990, final SP=0003:00051990.
* Microsoft's `c_common` 704-byte segment isn't integrated into R3G combined output segments. R3G allocates COMDEF storage separately.
* R3G code/data/BSS region ordering, group frames, alignment padding and start/stack symbol calculation are provisional; do not write fixups using these addresses.
* SarienPM imports PMWIN and QUECALLS dynamically; no PMWIN.LIB or QUECALLS.LIB should be invented.

## Reference LE facts

* hi.exe 11,806 bytes; entry 1:20, stack 2:2810, 2 objects.
* CMD32 91,231 bytes; entry 1:B9F4, stack 3:51990, 3 objects.
* Console 18,527 bytes; entry 1:1718, stack 3:22A00, 3 objects.
* Sarien 107,612 bytes; entry 1:F550, stack 2:5FCB0, 2 objects (code object base 10000, data object base 30000).

## Verification status

GCC and Clang strict C89 compile; GCC and Clang hello layout outputs compare byte-for-byte. Complete R2–R3G regression suite and R3H reference checks pass. This milestone adds verified instrumentation and reference comparisons, **not** Microsoft-equivalent final layout or a new linked executable.
