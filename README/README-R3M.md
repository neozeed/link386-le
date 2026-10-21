# LINK386 R3M — Group-aware final-address candidates

R3L is frozen. This standalone R3M branch extends its C89 planner with virtual-address candidates for each combined segment, individual module contribution, public symbol, and communal BSS symbol. It reports group anchors without conflating unrelated group names.

The object basing rule (0x10000, 0x20000, 0x30000) is a *declared local policy*, selected to compare against the supplied Microsoft examples; it is **not** an inferred general LINK386 placement algorithm. Mixed-width code receives a separate LE object candidate. No relocation is applied, no LE executable is emitted, no entry or stack symbol is claimed resolved, and the 16-byte CMD32 `_TEXT` tail remains unresolved. The output is `R3M ...` lines appended after R3G diagnostics.

Build and verify: `make link386r3g verify-r3m verify-r3l verify-r3j`.

Remaining work: canonical group anchor/base semantics, cross-module FIXUPP32 targets, true LE relocation records, DEF imports, correct initial EIP/ESP, and checking sizes/offsets against the Microsoft reference before an executable could be trusted.
