#!/usr/bin/env python3
"""Independent read-only FIXUPP decoder for the frozen Microsoft C/386 LIBC fixture.
Track THREAD definitions across FIXUPP records; print raw vs expanded LIDATA span.
"""
from pathlib import Path
from r3p_raw_audit import members

class Cursor:
 def __init__(self,p): self.p=p;self.i=0
 def byte(self):
  if self.i>=len(self.p):raise ValueError('truncated FIXUPP')
  v=self.p[self.i];self.i+=1;return v
 def index(self):
  a=self.byte();return ((a&127)<<8)|self.byte() if a&128 else a
 def word(self,n):
  if len(self.p)-self.i<n:raise ValueError('truncated displacement')
  a=int.from_bytes(self.p[self.i:self.i+n],'little');self.i+=n;return a

def decode(p,wide,ft,tt):
 c=Cursor(p);out=[]
 while c.i<len(p):
  start=c.i;x=c.byte()
  if not x&0x80:
   frame=bool(x&0x40);method=(x>>2)&7;slot=x&3
   index=c.index() if method<4 or (frame and method==4) else 0
   (ft if frame else tt)[slot]=(method,index)
   out.append(('THREAD','F' if frame else 'T',slot,method,index,start))
   continue
  offset=((x&3)<<8)|c.byte();loc=(x>>2)&15;dat=c.byte();fmethod=(dat>>4)&7;tmethod=dat&3
  if dat&0x80: fm,fi=ft[fmethod&3]
  else:
   fm=fmethod;fi=c.index() if fm<=3 else 0
  if dat&0x08: tm,ti=tt[tmethod]
  else: tm=tmethod;ti=c.index()
  disp=c.word(4 if wide else 2) if not dat&4 else None
  out.append(('FIXUP',offset,loc,(x>>6)&1,fm,fi,tm,ti,disp,start))
 return out

def timeline(path, wanted):
 seen=set()
 for name,base,rs in members(path):
  if name.lower() not in wanted:continue
  seen.add(name.lower());ft=[(None,None)]*4;tt=[(None,None)]*4;ext=0;last=None
  print('MODULE',name,'archive=0x%X'%base)
  for pos,t,p,end in rs:
   if t in (0x8c,0xb4):
    c=Cursor(p);names=[]
    while c.i<len(p):
     n=c.byte();s=c.p[c.i:c.i+n];c.i+=n;c.index();names.append(s.decode('latin1'))
    ext+=len(names)
    print(' EXT',hex(pos-base),'total',ext,'names',','.join(names))
   if t in (0xa0,0xa1,0xa2,0xa3):
    c=Cursor(p);seg=c.index();off=c.word(4 if t&1 else 2)
    last=(t,seg,off,len(p)-c.i,pos-base)
    print(' DATA',hex(pos-base),'type=%02X'%t,'seg',seg,'off',hex(off),'encoded',len(p)-c.i)
   if t in (0x9c,0x9d):
    print(' FIXUP_RECORD',hex(pos-base),'prior',last)
    for e in decode(p,t&1,ft,tt):
     print(' ',e)
     if e[0]=='FIXUP' and e[6]==2 and e[7]>ext:print('  EXTERNAL_INDEX_OUT_OF_RANGE',e[7],'>',ext)
  print(' END_THREAD_STATE',ft,tt)
 assert seen==wanted,(seen,wanted)

if __name__=='__main__':
 import sys
 timeline(Path(sys.argv[1] if len(sys.argv)>1 else Path(__file__).resolve().parents[1]/'fixtures/LIBC.LIB'),{'crt0.asm','chkstk.asm','heapgrow.c'})
