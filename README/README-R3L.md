# LINK386 R3L — GROUP identity preservation

R3K is frozen and unmodified. R3L is a separate C89 source tree.

R3L fixes the conservative combination planner's GROUP comparison: the prior code reconstructed group identity indirectly from the owner module and numeric segment index. It now stores the canonical GRPDEF group name on each output-segment candidate and uses that name directly to prevent cross-group coalescing.

R3L also prints `GROUP-MEMBER` diagnostics and a count of distinct observed group names. These are *identity* reports, not final LE object numbers or selector assignments. GROUP layout, contribution ordering within a group, and frame-relative relocation computation are not yet complete.

## Verification

`make link386r3g verify verify-r3g verify-r3i verify-r3j verify-r3k verify-r3l`

The new R3L fixture constructs two valid 32-bit OMF files with the same segment name/class/combine, but two different GRPDEF names. The planner must keep them separate. Renaming both GRPDEF records to the same name must permit them to combine. A real CMD32 check validates that `DGROUP` survives in the output report.

## Outstanding blockers

- CMD32 `_TEXT` is still 16 bytes short relative to Microsoft's MAP; no bytes were invented.
- R3L does not yet create a true group-wide contiguous offset address space for its relocation emitter.
- No cross-object fixup application, import fixup generation, or genuine executable linking is claimed.
- The R3A bounded synthetic LE writer remains unchanged.
