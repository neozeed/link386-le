#!/usr/bin/env python3
"""Diagnostic-only byte masks vs Microsoft reference, never used as build input."""
from pathlib import Path
from collections import Counter,defaultdict
import re,sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from r4h_compare import extract
P=Path(__file__).resolve().parents[1]
pat=re.compile(r'R4C PLACE module=(\S+) seg=(\S+) group=(\S*) object=(\d+) offset=([0-9A-Fa-f]+) size=(\d+)')
ref=extract(P/'fixtures/sarien/sarien.exe')
ours=extract(P/'build/sarien-r4g.exe')
masks=[(P/'build'/('object%d-r4f-r4g-mask.bin'%i)).read_bytes() for i in (1,2)]
rows=[]
for m,s,g,o,p,z in pat.findall((P/'build/r4g-build.log').read_text()):rows.append(dict(mod=m,seg=s,group=g,obj=int(o),old=int(p,16),size=int(z)))
for obj in (1,2):
 src=ours[obj-1]; dst=ref[obj-1]; mask=masks[obj-1]
 for item in (row for row in rows if row['obj']==obj):
  old=item['old']; sz=item['size']; cand=Counter(); occurrences=defaultdict(int)
  if old+sz>len(src) or sz<=0:continue
  for length in (12,8,6):
   # spread candidate windows through segment, not only leading instructions
   for relative in range(0,max(1,sz-length+1),max(1,min(8,sz//20))):
    at=old+relative
    if at+length>old+sz or any(b!=1 for b in mask[at:at+length]):continue
    needle=src[at:at+length]
    if len(set(needle))<4 or needle.count(0)>length//3:continue
    ix=0;n=0
    while True:
     ix=dst.find(needle,ix)
     if ix<0:break
     base=ix-relative
     if base>=0 and base+sz<=len(dst):
      cand[base]+=length
      occurrences[base]+=1
     ix+=1;n+=1
     if n>100:break
   if cand: break
  rankings=[]
  for base,weight in cand.most_common(15):
   pairs=[(i,src[old+i],dst[base+i]) for i in range(sz) if mask[old+i]==1]
   total=len(pairs);match=sum(x==y for _,x,y in pairs)
   rankings.append((match/total if total else 0,match,total,weight,base))
  rankings.sort(reverse=True)
  item['rankings']=rankings[:3]
  if rankings:
   top=rankings[0]; next_=rankings[1] if len(rankings)>1 else (0,0,0,0,-1)
   item['candidate']=top[4] if top[1]>=max(6,top[2]*.85) and (top[0]>next_[0]+.20 or top[1]-next_[1]>=5 or not next_[2]) else None
  else: item['candidate']=None
  if old>=0xf000 if obj==1 else old>=0x2a00:
   x=item['candidate'];detail=' '.join('base=%05X %d/%d wt=%d'%(r[4],r[1],r[2],r[3]) for r in rankings)
   print('OBJECT=%d MODULE=%s SEG=%s SIZE=%d OLD=%05X BEST=%s %s'%(obj,item['mod'],item['seg'],sz,old,'%05X'%x if x is not None else '?',detail))
