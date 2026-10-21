#!/usr/bin/env python3
"""Diagnostic comparison: R3G combined-segment sizes versus Microsoft MAP entries."""
import collections,re,subprocess,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
from r3h_oracle import inspect_map
if len(sys.argv)<4:raise SystemExit('usage: compare_r3g.py MAP OBJECT... LIBC.LIB OS2386.LIB')
m=inspect_map(sys.argv[1]); cmd=[str(Path(__file__).resolve().parents[1]/'link386r3g')]+sys.argv[2:]
r=subprocess.run(cmd,capture_output=True,text=True,check=True)
pat=re.compile(r'^  OUTSEG \d+ name=(\S+) class=(\S+) use32=(\d+) combine=(\d+) align=(\d+) offset=([0-9A-F]+) size=(\d+)')
planned=collections.defaultdict(int)
for line in r.stdout.splitlines():
 q=pat.match(line)
 if q: name,cls,use,comb,al,off,size=q.groups();planned[(name,cls,int(use))]+=int(size)
reference=collections.defaultdict(int)
for s in m['segments']:reference[(s['name'],s['cls'],s['bits']==32)]+=s['size']
keys=sorted(set(planned)|set(reference))
print('R3H DIAGNOSTIC: R3G SEGMENTS vs ORIGINAL MICROSOFT MAP')
print('NOTE: arithmetic comparison only; no claim of identical object placement.')
print('name class 32-bit R3G-size MAP-size delta')
matched=0
for name,cls,use in keys:
 p=planned.get((name,cls,use));v=reference.get((name,cls,use))
 if p is not None and v is not None and p==v:matched+=1
 print('%-22s %-10s %d %10s %10s %s'%(name,cls,use,str(p) if p is not None else 'MISSING',str(v) if v is not None else 'MISSING',str(p-v) if p is not None and v is not None else '-'))
print('SUMMARY matching-size-keys=%d/%d planned-keys=%d reference-keys=%d'%(matched,len(keys),len(planned),len(reference)))
