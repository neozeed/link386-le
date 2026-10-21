#!/usr/bin/env python3
from pathlib import Path
from r3s_le_fixups import decode
p=Path(__file__).resolve().parents[1]/'fixtures/hi-reference.exe'
d=decode(p)
assert len(d['objects'])==2
assert len(d['pages'])==3
assert d['modules']==['DOSCALLS']
assert sum(map(len,d['pages']))==78
assert len(d['sites'])==171
assert sum(x[3]==0 for x in d['sites'])==159
assert sum(x[3]!=0 for x in d['sites'])==12
assert d['objects'][0]['base']==0x10000
assert d['objects'][1]['base']==0x20000
print('R3S LE oracle: PASS (78 records, 159 internal + 12 import sites)')
