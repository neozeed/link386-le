#!/usr/bin/env python3
"""R4C: independent object-1 _TEXT layout from selected OMF, no Microsoft EXE needed.
Placement relies on explicit small/flat C/386 hello profile: initial offset 0x10;
selected _TEXT contributions in archive selection order; OMF alignment honored.
Relocations are NOT applied; mask bytes needing fixups for later work.
"""
from pathlib import Path
import csv,collections,hashlib,sys
ROOT=Path(__file__).resolve().parents[1]
ALIGN={0:1,1:1,2:2,3:16,4:256,5:4,6:4096}
def tab(p):
 with p.open(newline='') as f:return list(csv.DictReader(f,delimiter='\t'))
def run(out):
 c=tab(out/'contributions.tsv');f=tab(out/'fixups.tsv');obj=bytearray(0x10000);mask=bytearray(0x10000);offset=0x10;placed={};rows=[]
 for s in c:
  if s['name']!='_TEXT':continue
  align=ALIGN[int(s['align'])];assert align and (align&(align-1))==0
  offset=(offset+align-1)&~(align-1);size=int(s['size']);assert offset+size<=len(obj)
  placed[(s['member'],s['seg'])]=offset
  if size:
   source=(ROOT/s['file']).read_bytes();assert len(source)==size
   obj[offset:offset+size]=source;mask[offset:offset+size]=b'\x01'*size
  rows.append((int(s['member']),s['module'],int(s['seg']),offset,size,align))
  offset+=size
 # Mark all OMF fixup bytes as not independently relocatable; but retain original source bytes.
 for x in f:
  key=(x['member'],x['seg']);loc=int(x['loc']);width={0:1,1:2,2:2,3:2,9:4,11:4}.get(loc)
  if key not in placed or width is None:continue
  at=placed[key]+int(x['segment_site']);assert at+width<=offset,(x['module'],at,width,offset)
  mask[at:at+width]=b'\x02'*width
 obj=obj[:offset];mask=mask[:offset];(out/'object1-unrelocated.bin').write_bytes(obj);(out/'object1-mask.bin').write_bytes(mask)
 with (out/'object1-layout.tsv').open('w') as fp:
  fp.write('member\tmodule\tseg\tobject_offset\tvirtual_address\tsize\talign\n')
  for i,m,j,p,n,a in rows:fp.write(f'{i}\t{m}\t{j}\t{p}\t{0x10000+p}\t{n}\t{a}\n')
 print(f'R4C CODE BUILT size={len(obj)} source_bytes={mask.count(1)} unlinked_fixup_bytes={mask.count(2)} no_source={mask.count(0)} contributions={len(rows)} sha256={hashlib.sha256(obj).hexdigest()}')
 return obj,mask
if __name__=='__main__':run(Path(sys.argv[1] if len(sys.argv)>1 else ROOT/'build/r4c'))
