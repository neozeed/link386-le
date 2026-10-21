# LINK386 R4A — Complete contribution-ledger checkpoint

## Baseline and status
This release is derived from frozen R3Z, without altering the production linker C source or the R3Z fixtures. **It does not yet link `hi.exe`.** Executable generation and OMF FIXUPP writeback are still unproven. The previous R3Z engineering handoff is retained verbatim.

## New implementation
`tests/r4a_ledger.py` generates `r4a-ledger/segments.csv`, `omf_events.csv`, `fingerprint_anchors.csv`, and `summary.json` from the actual hi.obj, LIBC.LIB, R3N report and independent original Microsoft LE oracle.

The ledger preserves the original library member name, archive offset, segment index, name, class, combine/alignment/use32 attributes, GRPDEF membership, OMF data-record offset and data/FIXUPP chronology. It deliberately records *encoded* LIDATA payload size rather than pretending this is the expanded size. FIXUPP events retain the preceding data record but do not assume a valid or in-bounds fixup coordinate.

R3Y anchors are separately carried forward as confidence-ranked fingerprint evidence; an anchor is NOT a proven contribution length, linker ordering rule, or safe relocation address. Ambiguities such as the identical `fmsghdr.asm`/`nmsghdr.asm` candidate base are preserved.

## Known coverage limitation
The R3N relocation report identifies 31 distinct module names, whereas prior R3C/R3E link-plan reports selected 39. Accordingly, R4A's 31-module ledger is a **report-scoped subset**, not the complete 39-module selected-link closure. The next milestone should derive selection order directly from the OMF archive resolver, including members without relocations. The ledger currently records FIXUPP **records**, not individual fully decoded FIXUP subrecords or verified spans.

## Verification
Run `make verify-r4a` for the ledger, `make verify-r3z verify-r3s` for frozen prior oracles. The read-only generated ledger must not be confused with the LE writer.

## Remaining path to a real `hi.exe`
1. Obtain full 39-module selection order and complete contribution ledger from the linker, not only R3N textual classification.
2. Decode each OMF FIXUPP subrecord and expanded LIDATA spans independently with safe attribution.
3. Reconstruct object placement of initialized contents ahead of startup code and of heap `_TEXT` in object 2. Do not patch in `+0x1010`.
4. Reconcile disputed CRT/heapgrow/chkstk cases and verify group frame/target semantics.
5. Only then enable whitelisted cross-module fixup writes and LE import/internal fixup tables, validating against Microsoft's 159+12 sites.
