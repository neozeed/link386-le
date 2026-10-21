#!/usr/bin/env python3
"""R4F structural checks; reference for comparison only, never construction."""
from pathlib import Path
import hashlib
P=Path(__file__).resolve().parents[1]
def le(path):
 b=path.read_bytes(); off=int.from_bytes(b[60:64],'little');assert b[off:off+2]==b'LE'
 h=b[off:];u=lambda x:int.from_bytes(h[x:x+4],'little')
 objects=[(u(u(0x40)+24*i),u(u(0x40)+24*i+4),u(u(0x40)+24*i+12),u(u(0x40)+24*i+16)) for i in range(u(0x44))]
 pages=[h[u(0x48)+4*i+2] for i in range(u(0x14))]
 return b,h,u,objects,pages
out=P/'build/sarien-r4f.exe';ref=P/'fixtures/sarien/sarien.exe'
assert out.is_file() and ref.is_file()
b,h,u,ob,pm=le(out);br,hr,ur,obr,pr=le(ref)
assert ob[:1]==obr[:1] and ob[1][:3]==obr[1][:3]
assert ob[1][3]==4 and obr[1][3]==66 # unresolved virtual page-table extent
assert ob==[(74844,65536,1,19),(392368,196608,20,4)]
assert pm==pr
for k in (0x10,0x14,0x18,0x1c,0x20,0x24,0x28,0x2c):assert u(k)==ur(k),hex(k)
assert u(0x74)==ur(0x74)==4
assert b.find(b'PMWIN')>=0 and b.find(b'QUECALLS')>=0 and b.find(b'PMGPI')>=0
assert u(0x44)==2 and len(b)>100000
assert len(b)!=len(br)
print('R4F STRUCTURAL PASS objects=%s physical_pages=%d data_page_entries=%d entry=%08X stack=%08X dlls=%d' % (ob,u(0x14),ob[1][3],u(0x1c),u(0x24),u(0x74)))
print('R4F STATUS: independently generated; reference byte mismatches remain; Windows runtime NOT proven')
print('R4F OUTPUT SHA256 '+hashlib.sha256(b).hexdigest())
