#!/usr/bin/env python3
"""Compare complete LE fixup semantics; reference is read after source-only build."""
from pathlib import Path
from collections import Counter
from verify_r4e_fixups import read_fixups
P=Path(__file__).resolve().parents[1]
reference,n_ref=read_fixups(P/'fixtures/sarien/sarien.exe')
actual,n_act=read_fixups(P/'build/sarien-r4g.exe')
extra=actual-reference
missing=reference-actual
matched=reference&actual
print('R4H FIXUPP SITE COMPARISON')
print('Microsoft records=%d sites=%d'%(n_ref,sum(reference.values())))
print('R4G records=%d sites=%d'%(n_act,sum(actual.values())))
print('MATCH sites=%d MISMATCH extra=%d missing=%d'%(sum(matched.values()),sum(extra.values()),sum(missing.values())))
for title,coll in [('MATCHED',matched),('EXTRA',extra),('MISSING',missing)]:
 for obj in (1,2):
  sub=Counter({k:v for k,v in coll.items() if k[0]==obj})
  imports=sum(v for k,v in sub.items() if k[4]==1)
  internal=sum(v for k,v in sub.items() if k[4]==0)
  print('%s object=%d internal=%d imported=%d'%(title,obj,internal,imports))
 for k,v in list(sorted(coll.items(),key=lambda p:(p[0][0],p[0][1],str(p[0][5]))))[:12]:
  print('  %s object=%d at=%08X type=%d target=%s count=%d'%(title,k[0],k[1],k[2],str(k[5]),v))
assert sum(reference.values())==sum(actual.values())==2789
assert sum(extra.values())==sum(missing.values())==1366
print('R4H FIXUP COMPARISON PASS (mismatches are OPEN issues, not a clean-link claim)')
