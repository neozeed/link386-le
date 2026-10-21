# LINK386 R4D — Source-only C/386 LE executable production

Date: 2026-10-17. Status: **BUILD_AND_ORACLE_VERIFIED / WINDOWS_RUNTIME_UNTESTED**.

## Frozen baselines

* R4B is **LIVE-PROVEN, FROZEN and reference-assisted**. Its modified `hi-yo-r4b.exe` was successfully run in os2host32 on Windows and printed `yo!`. The original Microsoft LE was its runtime/fixup scaffold. R4D does not change R4B.
* R4C is frozen, source-only runtime-object construction, package SHA256 `a876ee8bbe86979e1a8817a013c741e8769e389cadbc89a2b22d0a0c4bb30580`. R4D is a fork. R4C materialized 6,174/10,256 object bytes but still had 61 mismatching relocation candidates and 13 ordinary data mismatches.

## R4D implemented

`src/link386r4d.c` is a **strict ISO C89, standalone, source-only LE executable emitter** for the exact verified C/386 hello link profile. It takes only `hi.obj`, `LIBC.LIB`, and `OS2386.LIB`. **No existing Microsoft EXE is used in the executable-building path.** The original `hi-reference.exe` is used in the verification script *after* execution of the new source-only linker.

Example:

```sh
make link386r4d
./link386r4d -o build/r4d/hi-r4d-source-only.exe fixtures/hi.obj fixtures/LIBC.LIB fixtures/OS2386.LIB
./link386r4d -o build/r4d/hi-yo-r4d-source-only.exe fixtures/hi-yo.obj fixtures/LIBC.LIB fixtures/OS2386.LIB
make verify-r4d
```

Output `hi-r4d-source-only.exe`: **12,318 bytes**, SHA256 `c5409b6a159cbf1b72800509873989f869c1aba42ec1ce76f288480676650038`.

Source-changed `hi-yo-r4d-source-only.exe`: **12,318 bytes**, SHA256 `c434aa527a7b56db38bcb7ce1aa982d608c07d31c4768f4ed4cc74e850dce36c`.

The new LE differs in file size from Microsoft's 11,806-byte output because R4D serializes one loader record per source fixup rather than grouping multiple source sites for a shared target.

## Fixes discovered and implemented

1. **OMF LEXTDEF32 (record 0xB5)** was ignored by R2–R4C. This dropped local external symbols (`_before`, `flsall`, `_heap_new_region`) from the external-index namespace; that caused most of the 61 incorrect relocations and the false heapgrow.c external-index-13 mystery. R4D supports both `0xB4` and `0xB5`, preserving external-index order.
2. **LIDATA32 FIXUPP coordinates refer into compressed encoded literal bytes, not simply to expanded data offsets.** R4D retains encoded LIDATA and traverses nested/repeated blocks, expanding one FIXUPP record to every actual relocation source site. In `crt0.asm`, encoded site 13 resolves to object2 offset `0x58` (not `0x65`). In `cmiscdat.asm`, encoded site 13 resolves to six DWORDs at object2 offsets `0x3F0` through `0x404`. Site-count growth is exactly five (301 input FIXUPPs / 306 expanded sites).
3. **Communal BSS order/alignment:** the verified hello profile places 192-byte `__heap_regions` at `0x20550`, followed by 512-byte `__bufin` at `0x20610`. R4D's private layout uses 16-byte `c_common` alignment and reverses communal declaration order in its address resolver. This is a validated **hello-profile placement policy** and must not be imposed on unrelated binaries without additional tests.
4. **`_end` boundary** is the start of STACK (`0x20810`), not its end (`0x22810`). In `chkstk.asm`, an OMF displacement `0x400` yields the Microsoft relocation value `0x20C10`.
5. **OMF frame=EXT(target)** for `cmiscdat.asm` encodes a target-object-relative value `0x234` (six locations), not `0x10234`; these six values are **not** loader relocation sites.
6. **LE page-map/loader-fixup ordering**: logical LE pages `[object1 page1, object1 page2, object2 page1]` map to physical data pages `[2,3,1]`, while fixup records in this Microsoft LE are grouped by physical page order `[object2 data, object1 code page1, object1 code page2]`. Earlier R3S/R3U site-object inferences that ignored this were incorrect. The new emitter preserves this layout.

## Proven comparison results

* 39 OMF-selected modules; 120 contributions; two LE object virtual sizes `0x181E` and `0x2810` at preferred bases `0x10000` and `0x20000`.
* 301 FIXUPP input records, 306 expanded source locations; 294 statically applied or source-resolved locations (165 absolute, 129 relative), 12 DLL imports deferred to the LE loader; zero other unresolved.
* Of the 165 absolute OMF source sites, 6 `cmiscdat.asm` frame-relative sites are fully linked statically. **159 remaining OFF32 sites require LE loader fixups.** All 12 DOSCALLS REL32 sites are emitted as import fixups.
* Every **16,430 bytes** of virtual object content in the independently constructed executable (6,174 code + 10,256 data/BSS/stack) agrees with Microsoft's `hi.exe` when its unmapped BSS/stack tail is zero-filled.
* The **full set of 171 LE loader sites, source kinds, target objects/offsets and DOSCALLS ordinals** agrees exactly with the Microsoft reference when the original LE physical-page mapping is honoured.
* GCC and Clang `-O2 -std=c89 -pedantic -Wall -Wextra -Werror` builds pass, and produce byte-identical output. Repeated runs deterministic. Changed `hi.obj` outputs `yo!` data change at exactly object2 offsets `0x44/0x45`; corrupt source OMF checksum rejected.
* Old R4B `make verify-r4b` passes on this fork. `make verify` and R3G/R3I/R3J/R3L/R3M/R3N pass. **Historical R3O assertions about heapgrow index 13 becoming invalid are now expected to fail**—the formerly missing `LEXTDEF32` is correctly included.

## Known boundaries / limitations

This is **not yet a general LINK386 replacement**. The writer deliberately checks exact hello-profile object sizes and base addresses, fixes the 16-byte code prefix, uses exactly two LE objects and three physical data pages, assumes one DOSCALLS import module and 8 KB bundled default stack, and supports OMF fixup forms exercised by this profile. It does not currently implement general OBJ/DEF/MAP/linker option processing, arbitrary OMF communal placement, mixed 16/32-bit CMD32 output, PM imports, or compressed/optimized fixup source lists.

The emitted `.exe` was **not executed on Windows in this environment**. Parsing and comparing all bytes/fixup targets proves a strong static oracle, but the milestone can only be marked **LIVE-PROVEN** after a separate os2host32 runtime test.

Windows test commands:

```bat
\OS2\os2host32.exe --scan hi-r4d-source-only.exe
\OS2\os2host32.exe --run hi-r4d-source-only.exe
\OS2\os2host32.exe --run hi-yo-r4d-source-only.exe
```

Expected printed program messages are `hi!` and `yo!`, respectively, if the independently emitted LE loader metadata is compatible with the live host.

## Recommended next stage R4E

First get Windows execution confirmation for **unscaffolded** R4D; fix any LE header/loader compatibility issues without borrowing the Microsoft EXE. Freeze only after live testing. Then generalize the 2-object hello-only placement profile to handle the supplied CMD32 console test, including class ordering, mixed-width objects, `/STACK`, `.DEF` import bindings, and MAP output, one property at a time. Keep R4B/R4C/R4D source snapshots immutable.
