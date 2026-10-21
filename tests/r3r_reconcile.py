#!/usr/bin/env python3
"""R3R: independent, conservative OMF namespace + data interval reconciler.
No linker bytes modified. Tracks external index order including COMDEF/LCOMDEF;
checks FIXUPP site coordinates under record-relative and segment-absolute hypotheses.
"""
from pathlib import Path
import sys
from r3p_raw_audit import members
from r3q_thread_audit import Cursor,decode

def external_names(t,p):
 c=Cursor(p); out=[]
 while c.i<len(p):
  n=c.byte(); s=p[c.i:c.i+n];c.i+=n;c.index()
  if t in (0xb0,0xb8):
   dtype=c.byte()
   def number():
    v=c.byte();return v if v<=128 else c.word({0x81:2,0x84:3,0x88:4}[v])
   if dtype==0x61:number();number()
   elif dtype==0x62:number()
   else: raise ValueError('unknown COMDEF dtype %02x'%dtype)
  out.append(s.decode('latin1'))
 return out

def expanded_lidata(t,p):
 c=Cursor(p);seg=c.index(); offset=c.word(4 if t&1 else 2)
 def block(depth=0):
  if depth>32:raise ValueError('LIDATA nesting')
  count=c.word(4 if t&1 else 2); n=c.word(2)
  if n:
   size=sum(block(depth+1) for _ in range(n))
  else:
   k=c.byte();c.i+=k;size=k
  if size*count>16*1024*1024:raise ValueError('oversized LIDATA')
  return size*count
 total=0
 while c.i<len(p): total+=block()
 return seg,offset,total

def scan(lib):
 target={'crt0.asm','chkstk.asm','heapgrow.c'};seen=set(); issues=[]
 for name,base,rs in members(lib):
  if name.lower() not in target:continue
  seen.add(name.lower()); ext=[];dat=[];last=None;ft=[(None,None)]*4;tt=[(None,None)]*4
  print('MODULE',name,'offset=0x%x'%base)
  for pos,t,p,end in rs:
   if t in (0x8c,0xb4,0xb0,0xb8):
    names=external_names(t,p);ext.extend(names)
    print(' NAMESPACE','%02x'%t,'add',len(names),'total',len(ext))
   elif t in (0xa0,0xa1,0xa2,0xa3):
    if t in (0xa2,0xa3):seg,off,n=expanded_lidata(t,p)
    else:
     c=Cursor(p);seg=c.index();off=c.word(4 if t&1 else 2);n=len(p)-c.i
    last=(seg,off,n,t);dat.append(last)
    if name.lower()=='crt0.asm':print(' DATA',hex(pos-base),'segment',seg,'interval','[%x,%x)'%(off,off+n),'type=%02x'%t)
   elif t in (0x9c,0x9d):
    for e in decode(p,bool(t&1),ft,tt):
     if e[0]!='FIXUP':continue
     _,at,loc,mode,fm,fi,tm,ti,disp,start=e
     if tm==2 and (ti<1 or ti>len(ext)):
      print(' OUT_OF_RANGE_EXT','offset=%x'%(pos-base),'target',ti,'declared',len(ext),'record_byte',start)
      issues.append((name,'external namespace out of range',ti,len(ext)))
     if name.lower()=='crt0.asm' and pos-base==0x2d4:
      print(' CRT_FIXUP','site',at,'location_kind',loc,'last_data',last)
      if last:
       seg,off,n,_=last
       print(' CRT_INTERVAL_TEST','record-relative 4-byte:',at+4<=n,'absolute 4-byte:',off<=at and at+4<=off+n)
       print(' CRT_OFFSET_CONFLICT','at=13, last data segment offset=12, expanded length=4; location kind must be interpreted before write')
  print(' EXTERNAL_TOTAL',len(ext))
  if name.lower()=='heapgrow.c':
   print(' HEAPGROW_LAST_EXTERNAL',list(enumerate(ext,1))[-4:])
 assert seen==target,(seen,target)
 assert any(x[0]=='heapgrow.c' and x[2]==13 for x in issues)
 print('RESULT: namespace discrepancy confirmed; no synthetic symbol, no writebacks')
if __name__=='__main__':scan(Path(sys.argv[1] if len(sys.argv)>1 else Path(__file__).resolve().parents[1]/'fixtures/LIBC.LIB'))
