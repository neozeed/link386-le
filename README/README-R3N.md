# LINK386 R3N — candidate 32-bit relocation expressions

R3M is frozen separately; R3N is its independent successor.

`link386r3n` builds atop the existing OMF reader and group-aware layout planner. It walks selected modules' FIXUPP/FIXUPP32 entries, associates source data records and contribution VAs, and attempts target resolution against selected segment contributions, group anchors, public definitions, communal allocations, and import definitions.

R3N labels 32-bit offset results **CANDIDATE** because Microsoft frame/target and displacement/addend rules are not fully implemented or independently verified. It does **not** modify the input bytes, apply fixups, emit LE records, or generate runnable executables. `IMPORT` marks runtime LE fixups that require the later import-table emitter. `DEFER` and `BLOCK` preserve unsupported or invalid cases for inspection. The classification does not fully model `LIDATA` repeated-fixup multiplicity; do not equate record counts with final LE sites.

## Reproduction

    make link386r3n verify-r3n
    ./link386r3n fixtures/hi.obj fixtures/LIBC.LIB fixtures/OS2386.LIB > r3n-hi-report.txt

Run the existing regression targets with `make verify verify-r3g verify-r3i verify-r3j verify-r3k verify-r3l verify-r3m`.

### Remaining barriers

* Resolve blocked target cases and audit frame methods, group-based addressing, and relocation addends against Microsoft's LE oracle.
* Translate `LIDATA` fixup locations into expanded output sites.
* Support 16-bit, far-pointer and segment selector fixup forms.
* Generate actual LE import/fixup tables, and validate entry/stack and code tail.
* Treat actual LE emission and successful guest execution as separate future milestones.
