#!/usr/bin/env python3
"""R4G bounded Sarien contribution-order experiment; never a runtime-success oracle."""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]

def extract(path):
    b=path.read_bytes(); h=b[int.from_bytes(b[60:64],'little'):]
    u=lambda off:int.from_bytes(h[off:off+4],'little')
    p=[h[u(0x48)+4*i+2] for i in range(u(0x14))]
    result=[]
    for i in range(u(0x44)):
        off=u(0x40)+24*i; n=u(off); start=u(off+12); count=u(off+16)
        out=bytearray(n)
        for j in range(min(count,len(p)-start+1)):
            phys=p[start+j-1]
            if phys:
                at=u(0x80)+(phys-1)*4096
                countbytes=min(4096,n-j*4096)
                out[j*4096:j*4096+countbytes]=b[at:at+countbytes]
        result.append(out)
    return result

baseline=extract(ROOT/'build/sarien-r4f.exe')
trial=extract(ROOT/'build/sarien-r4g.exe')
reference=extract(ROOT/'fixtures/sarien/sarien.exe')
lines=(ROOT/'build/r4g-build.log').read_text()
found={}
for mod in ('crt0.asm','crt0dat.asm','fclose.c','fopen.c'):
    x=re.search(r'R4C PLACE module='+re.escape(mod)+r' seg=_TEXT .*?offset=([0-9A-Fa-f]+)',lines)
    assert x,mod
    found[mod]=int(x.group(1),16)
assert found=={'crt0.asm':0xF550,'crt0dat.asm':0xF628,'fclose.c':0xF740,'fopen.c':0xF7FC}, found
assert len(trial)==len(reference)==2
for name,images in [('R4F baseline',baseline),('R4G experiment',trial)]:
    for index,(actual,ref) in enumerate(zip(images,reference),1):
        mask=(ROOT/'build'/('object%d-r4f-mask.bin'%index)).read_bytes() if name=='R4G experiment' else (ROOT/'build'/('object%d-r4f-baseline-mask.bin'%index)).read_bytes()
        assert len(actual)==len(ref)==len(mask)
        bad=[p for p in range(len(actual)) if actual[p]!=ref[p] and mask[p]==1]
        print('%s object%d ordinary_differences=%d first=%s' %(name,index,len(bad), hex(bad[0]) if bad else 'NONE'))
        if name=='R4G experiment' and index==1:
            assert bad[0]==0xF848
            assert len(bad)<9649
        if name=='R4G experiment' and index==2:assert bad[0]==0x2B18
print('R4G ORDERING EXPERIMENT PASS — partial correctness only; NOT runtime-proven')
