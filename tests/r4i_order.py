#!/usr/bin/env python3
"""R4I post-build reference-only source-window placement inference.
Never used by linker or executable construction.
"""
from pathlib import Path
from collections import Counter
import re, sys
sys.path.insert(0,str(Path(__file__).parent))
import contextlib, io
with contextlib.redirect_stdout(io.StringIO()):
 from r4h_compare import extract
ROOT=Path(__file__).resolve().parents[1]
log=(ROOT/'build/r4g-build.log').read_text()
pat=re.compile(r'R4C PLACE module=(\S+) seg=(\S+) group=(\S*) object=(\d+) offset=([0-9A-Fa-f]+) size=(\d+)')
ref=extract(ROOT/'fixtures/sarien/sarien.exe')
ours=extract(ROOT/'build/sarien-r4g.exe')
masks=[(ROOT/'build'/('object%d-r4f-mask.bin'%i)).read_bytes() for i in (1,2)]
entries=[]
for line in log.splitlines():
 m=pat.search(line)
 if m:
  mod,seg,group,obj,off,sz=m.groups();entries.append((mod,seg,int(obj)-1,int(off,16),int(sz)))
results=[]
for mod,seg,obj,off,size in entries:
 if size<32 or off+size>len(ours[obj]):continue
 src=ours[obj];target=ref[obj];mask=masks[obj]
 votes=Counter();windows=0
 # Multiple separated windows, only literal bytes unaffected by fixups.
 for rel in range(0,size-19,max(8,size//24)):
  a=off+rel
  if any(mask[a:a+16][i]!=1 for i in range(16)):continue
  needle=bytes(src[a:a+16])
  if needle.count(0)>8 or len(set(needle))<5:continue
  windows+=1
  start=0
  while True:
   at=target.find(needle,start)
   if at<0:break
   base=at-rel
   if base>=0 and base+size<=len(target):votes[base]+=1
   start=at+1
   if start>=len(target):break
 if not votes:continue
 rank=votes.most_common(3)
 base,score=rank[0]
 second=rank[1][1] if len(rank)>1 else 0
 trusted=score>=3 and score>=second*2 and windows>=4
 results.append((obj,off,base,size,mod,seg,score,windows,second,trusted))
print('R4I independent post-build fingerprint ordering; never used as linker input')
for obj in (0,1):
 found=[r for r in results if r[0]==obj]
 good=sorted((r for r in found if r[-1]),key=lambda r:r[2])
 print('OBJECT',obj+1,'eligible',len(found),'strong',len(good))
 for r in good:
  _,old,new,size,mod,seg,score,windows,second,_=r
  print('ANCHOR obj=%d module=%s seg=%s planned=%05X reference=%05X delta=%+d bytes=%d votes=%d/%d runner=%d' %(obj+1,mod,seg,old,new,new-old,size,score,windows,second))
 print('UNRESOLVED',sum(1 for r in found if not r[-1]))
print('R4I strong placements',sum(r[-1] for r in results),'of',len(results),'tested contribution segments')
