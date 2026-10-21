#!/usr/bin/env python3
"""Independent reference/linked output LE loader-site comparison for R4E-FIX1.
Uses official LE source-list and selector-no-target-offset rules. No reference
EXE is ever supplied to linker; references are read only after generation.
"""
from collections import Counter
from pathlib import Path
import struct,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'fixtures/cmd32/shell'

def u16(b,p):return struct.unpack_from('<H',b,p)[0]
def u32(b,p):return struct.unpack_from('<I',b,p)[0]
def read_fixups(path):
 b=path.read_bytes();h=u32(b,0x3c)
 if b[h:h+2]!=b'LE':raise ValueError('not LE')
 n=u32(b,h+0x14);pt=h+u32(b,h+0x48);np=u32(b,h+0x44)
 ot=h+u32(b,h+0x40);pg=h+u32(b,h+0x68);fr=h+u32(b,h+0x6c)
 mt=h+u32(b,h+0x70);mcount=u32(b,h+0x74)
 modules=[];p=mt
 for _ in range(mcount):
  length=b[p];p+=1;modules.append(b[p:p+length].decode('ascii').upper());p+=length
 # LE fixup pages are PHYSICAL pages, not logical object page ordinals.
 pageinfo=[]
 for obj in range(np):
  p=ot+24*obj;first=u32(b,p+12);pages=u32(b,p+16)
  for j in range(pages):
   if first-1+j>=n:break  # some historical objects map huge BSS virtually
   entry=pt+4*(first-1+j)
   physical=int.from_bytes(b[entry:entry+3],'big')
   if physical==0:continue  # uninitialized BSS/STACK logical page without physical bytes
   if physical<1 or physical>n:raise ValueError('invalid physical page')
   pageinfo.append((physical,obj+1,j*4096))
 pageinfo={physical:(obj,base) for physical,obj,base in pageinfo}
 offs=[u32(b,pg+4*i) for i in range(n+1)]
 if offs!=sorted(offs):raise ValueError('disordered fixup page offsets')
 result=[];count=0
 for phys in range(1,n+1):
  p=fr+offs[phys-1];end=fr+offs[phys]
  while p<end:
   start=p;src=b[p];flags=b[p+1];p+=2
   st=src&15
   if st not in (2,3,6,7,8):raise ValueError('unsupported source 0x%X at 0x%X'%(src,start))
   if (src&0x10) and st not in (2,3,6):raise ValueError('alias invalid')
   if src&0x20:cnt=b[p];p+=1;offsets=None
   else:offsets=[struct.unpack_from('<h',b,p)[0]];p+=2
   mode=flags&3
   if mode not in (0,1):raise ValueError('unsupported target mode')
   num_size=2 if flags&0x40 else 1
   idx=int.from_bytes(b[p:p+num_size],'little');p+=num_size
   if mode==0:
    if st==2:target=None
    else:
     sz=4 if flags&0x10 else 2
     target=int.from_bytes(b[p:p+sz],'little');p+=sz
    if idx<1 or idx>np:raise ValueError('invalid target object')
    target=('object',idx,target)
   elif mode==1:
    sz=4 if flags&0x10 else 1 if flags&0x80 else 2
    target=int.from_bytes(b[p:p+sz],'little');p+=sz
    if idx<1 or idx>len(modules):raise ValueError('invalid DLL')
    target=('import',modules[idx-1],target)
   if flags&4:raise ValueError('unexpected additive fixup')
   if src&0x20:
    offsets=[struct.unpack_from('<h',b,p+2*i)[0] for i in range(cnt)];p+=cnt*2
   if p>end:raise ValueError('record passes page')
   obj,base=pageinfo[phys]
   for off in offsets:
    result.append((obj,base+off,st,bool(src&0x10),mode,target))
   count+=1
  if p!=end:raise ValueError('bad page boundary')
 return Counter(result),count

def main():
 # Both commands use only the original OMF inputs and libraries. The Microsoft
 # executables are opened solely after the new output has been generated.
 from verify_r4e import do_link
 with tempfile.TemporaryDirectory(prefix='r4e-fix1-') as work:
  for typ,orig,new in [
   ('console','cmdos2_os2_console_test.exe','cmdos2_os2_console_test-r4e-fix1.exe'),
   ('full','cmd32os2_os2.exe','cmd32os2-r4e-fix1.exe')]:
   outdir=Path(work)/typ;outdir.mkdir()
   generated,ref,*_=do_link(outdir,typ)
   # The linker executable remains named link386r4e in FIX1; the outputs
   # here are compared to their exact original executable fixtures.
   reference,refcnt=read_fixups(BASE/orig)
   actual,gencnt=read_fixups(generated)
   extra=actual-reference;miss=reference-actual
   print('%s: Microsoft=%d records/%d sites; FIX1=%d records/%d sites; extra=%d missing=%d'%(
    typ,refcnt,sum(reference.values()),gencnt,sum(actual.values()),sum(extra.values()),sum(miss.values())))
   if extra or miss:
    print('EXTRA',list(extra.items())[:20]);print('MISSING',list(miss.items())[:20]);return 1
   types={r[2] for r in actual}
   assert types=={2,3,6,7,8},types
   assert all(r[3] for r in actual if r[2]==3), 'internal 16:16 pointer fixup not converted to selector'
   assert sum(v for r,v in actual.items() if r[2]==2)==14
   assert sum(v for r,v in actual.items() if r[2]==6)==7
  return 0
if __name__=='__main__':sys.exit(main())
