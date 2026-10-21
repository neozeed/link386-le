#!/usr/bin/env python3
"""R3V independent, read-only LEDATA provenance by unrelocated byte fingerprints.
Matches intact 12-byte windows; a negative match proves nothing about relocations.
"""
from pathlib import Path
from collections import defaultdict,Counter
from r3p_raw_audit import members, records
from r3s_le_fixups import decode,u32
ROOT=Path(__file__).resolve().parents[1]

def idx(b,p):
 x=b[p];return ( ((x&127)<<8)|b[p+1],p+2) if x&128 else (x,p+1)

def candidates(lib):
 for name,off,rs in members(lib):
  segs={}; ln=['']; segno=0
  for pos,t,p,end in rs:
   if t==0x96:
    q=0
    while q<len(p):
     n=p[q];q+=1;ln.append(p[q:q+n].decode('latin1'));q+=n
   if t in (0x98,0x99):
    segno+=1;q=1;attr=p[0];align=attr>>5
    if align==0:q+=3
    q+=4 if t==0x99 else 2
    ni,q=idx(p,q);ci,q=idx(p,q);oi,q=idx(p,q)
    segs[segno]=(ln[ni] if ni<len(ln) else '?',ln[ci] if ci<len(ln) else '?')
   if t in (0xa0,0xa1):
    si,q=idx(p,0);offset=int.from_bytes(p[q:q+(4 if t==0xa1 else 2)],'little');q+=4 if t==0xa1 else 2
    raw=p[q:]
    if len(raw)>=12:yield name,segs.get(si,('?', '?')),offset,raw

def scan():
 exe=ROOT/'fixtures/hi-reference.exe';lib=ROOT/'fixtures/LIBC.LIB'
 d=decode(exe);b=exe.read_bytes();data=u32(b,d['header']+0x80);ps=d['page_size']
 images=[]
 for o in d['objects']:
  chunks=[]
  for pg in range(o['first']-1,o['first']-1+o['pages']):
   chunks.append(b[data+pg*ps:data+(pg+1)*ps])
  images.append((o['index'],o['base'],b''.join(chunks)))
 print('R3V LEDATA FINGERPRINT CORRELATION -- READ ONLY')
 print('IMAGE data_offset=%X page_size=%d'% (data,ps))
 bymod=defaultdict(lambda: defaultdict(set));samples=Counter()
 for name,seg,offset,raw in candidates(lib):
  for i in range(0,len(raw)-11,4):
   sig=raw[i:i+12]
   if sig==bytes(12) or len(set(sig))<4:continue
   samples[name]+=1
   for n,base,body in images:
    at=body.find(sig)
    if at>=0 and body.find(sig,at+1)<0:
     # source contribution base candidate from within LEDATA
     bymod[name][(seg[0],n)].add(base+at-(offset+i))
 for name in sorted(bymod):
  rows=bymod[name]
  if name.lower() in {'crt0.asm','crt0dat.asm','nmsghdr.asm','lseek.asm','write.asm','heapgrow.c','heapinit.c'}:
   print('IMPORTER',name,'sample_windows',samples[name])
   for (seg,obj),bases in sorted(rows.items()):
    vals=','.join('%08X'%v for v in sorted(bases)[:8]);print(' ANCHOR segment=%s object=%d possible_segment_bases=%d values=%s'%(seg,obj,len(bases),vals))
 print('CAUTION: fingerprints only establish matching immutable byte windows, NOT relocation-site ownership or a proven segment base')
 return bymod
if __name__=='__main__':scan()
