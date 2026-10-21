from pathlib import Path
s=Path('r3k-reconciliation.txt').read_text()
assert 'TEXT-TAIL planned-end=0000F1C1 Microsoft-end=0000F1D1 missing=16' in s
assert 'cause=UNPROVEN' in s
assert 'MATCH c_common' in s
assert 'MATCH STACK' in s
assert 'MATCH $C20006_CODE16' in s
print('R3K MAP reconciliation regression PASS')
