#!/usr/bin/env python3
"""R4H independent reference comparison; never feeds reference into linking."""
from pathlib import Path
import re, collections, argparse
ROOT=Path(__file__).resolve().parents[1]
def extract(path):
 b=path.read_bytes(); l=int.from_bytes(b[60:64],'little'); assert b[l:l+2]==b'LE'
 h=b[l:]; u=lambda k:int.from_bytes(h[k:k+4],'little')
 pages=[h[u(0x48)+4*i+2] for i in range(u(0x14))]
 images=[]
 for i in range(u(0x44)):
  o=u(0x40)+i*24; n=u(o); start=u(o+12); count=u(o+16); data=bytearray(n)
  for j in range(min(count,len(pages)-start+1)):
   physical=pages[start+j-1]
   if physical:
    src=u(0x80)+(physical-1)*4096
    length=min(4096,n-4096*j)
    data[4096*j:4096*j+length]=b[src:src+length]
  images.append(data)
 return images
log=(ROOT/'build/r4g-build.log').read_text()
entries=[]
pat=re.compile(r'R4C PLACE module=(\S+) seg=(\S+) group=(\S*) object=(\d+) offset=([0-9A-Fa-f]+) size=(\d+)')
for line in log.splitlines():
 m=pat.search(line)
 if m:
  mod,seg,group,obj,off,size=m.groups()
  entries.append((int(obj),int(off,16),int(size),mod,seg,group))
ref=extract(ROOT/'fixtures/sarien/sarien.exe')
ours=extract(ROOT/'build/sarien-r4g.exe')
assert len(ref)==len(ours)==2
print('R4H Sarien contribution-based differential (reference read only after independent build)')
for obj in (1,2):
 a=ours[obj-1]; b=ref[obj-1]
 mask=(ROOT/'build'/('object%d-r4f-mask.bin'%obj)).read_bytes()
 assert len(a)==len(b)==len(mask)
 bad=[i for i in range(len(a)) if a[i]!=b[i] and mask[i]==1]
 owners=[]
 for at in bad:
  owner=next(((mod,seg,base,size) for oid,base,size,mod,seg,_ in entries if oid==obj and base<=at<base+size),None)
  owners.append(owner or ('UNATTRIBUTED','?',0,0))
 summary=collections.Counter((o[0],o[1],o[2],o[3]) for o in owners)
 print('OBJECT',obj,'ordinary_mismatches',len(bad),'first',hex(bad[0]) if bad else 'NONE')
 for (mod,seg,base,size),number in summary.most_common(25):
  first=next(i for i,owner in zip(bad,owners) if owner==(mod,seg,base,size))
  print('DIFF module=%s seg=%s contribution=%08X size=%d mismatched=%d first=%08X own=%02X ms=%02X' %(mod,seg,base,size,number,first,a[first],b[first]))
 if obj==2:
  print('FIRST DATA AREA:')
  for off in range(0x2B00,0x2B40,4):
   print('%05X ours=%s microsoft=%s mask=%s' %(off,a[off:off+4].hex(),b[off:off+4].hex(),mask[off:off+4].hex()))
assert any(x[3]=='crt0msg.asm' and x[0]==2 and x[1]==0x2B18 for x in entries)
print('R4H DIFFERENTIAL PASS')
