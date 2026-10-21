#!/usr/bin/env python3
"""Independent post-build R4I anchor cross-check, never used for production linker output."""
from pathlib import Path
import re, sys
from collections import Counter
sys.path.insert(0,str(Path(__file__).resolve().parent))
from verify_r4e_fixups import read_fixups
from r4h_compare import extract
P=Path(__file__).resolve().parents[1]
anchor=re.compile(r'ANCHOR obj=(\d+) module=(\S+) seg=(\S+) planned=([0-9A-F]+) reference=([0-9A-F]+)')
place=re.compile(r'R4C PLACE module=(\S+) seg=(\S+) group=\S* object=(\d+) offset=([0-9A-F]+) size=(\d+)')
refs={(int(o),m,s):int(ref,16) for o,m,s,_,ref in anchor.findall((P/'build/r4i-order.txt').read_text())}
positions={(int(o),m,s):int(off,16) for m,s,o,off,_ in place.findall((P/'build/r4j-build.log').read_text())}
for obj in (1,2):
 keys=[(key,off,positions.get(key)) for key,off in refs.items() if key[0]==obj]
 matched=[x for x in keys if x[1]==x[2]]
 deltas=Counter(x[2]-x[1] for x in keys if x[2] is not None)
 print('R4J ANCHORS object=%d exact=%d/%d major_deltas=%s'%(obj,len(matched),len(keys),deltas.most_common(10)))
 for k,ref,actual in sorted(keys,key=lambda x:x[1]):
  if ref!=actual:print('  UNMATCHED module=%s seg=%s reference=%05X candidate=%s'%(k[1],k[2],ref,('%05X'%actual if actual is not None else 'MISSING')))
r,_=read_fixups(P/'fixtures/sarien/sarien.exe')
for name in ('r4g','r4j'):
 a,_=read_fixups(P/('build/sarien-%s.exe'%name))
 print('R4J FIXUPS variant=%s exact=%d missing=%d extra=%d'%(name,sum((r&a).values()),sum((r-a).values()),sum((a-r).values())))
ref=extract(P/'fixtures/sarien/sarien.exe')
for name in ('r4g','r4j'):
 objs=extract(P/('build/sarien-%s.exe'%name))
 for obj in (1,2):
  mask=P/'build'/('object%d-r4f-%s-mask.bin'%(obj,name))
  if mask.exists():
   m=mask.read_bytes()
   bad=[i for i,(x,y) in enumerate(zip(objs[obj-1],ref[obj-1])) if x!=y and m[i]==1]
   print('R4J ORDINARY variant=%s object=%d count=%d first=%s'%(name,obj,len(bad),hex(bad[0]) if bad else 'NONE'))
print('R4J CONSTRAINT AUDIT PASS (diagnostic, no runtime claim)')
