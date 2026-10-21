#!/usr/bin/env python3
"""R4L non-oracle regression: OMF dependency-level CODE and linker hash COMMON.
Reference executable is opened ONLY AFTER both independent source builds.
This test is intended to fail when a newly proposed generic policy regresses.
"""
from pathlib import Path
from collections import Counter
import hashlib
import shutil
import subprocess
import tempfile
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tests'))
from verify_r4e_fixups import read_fixups
def extract(path):
    # Independent LE logical-to-physical page mapper; no R4G build logs.
    b=path.read_bytes()
    at=int.from_bytes(b[0x3c:0x40],'little')
    assert b[at:at+2]==b'LE'
    h=b[at:]
    u=lambda i:int.from_bytes(h[i:i+4],'little')
    count=u(0x14)
    pages=[h[u(0x48)+4*i+2] for i in range(count)]
    result=[]
    for i in range(u(0x44)):
        o=u(0x40)+24*i
        n,first,num=u(o),u(o+12),u(o+16)
        blob=bytearray(n)
        for j in range(min(num,len(pages)-first+1)):
            physical=pages[first+j-1]
            if physical:
                p=u(0x80)+(physical-1)*4096
                length=min(4096,n-4096*j)
                blob[j*4096:j*4096+length]=b[p:p+length]
        result.append(blob)
    return result

APP_NAMES=['agi','agi_v2','agi_v3','agi_v4','checks','cli','console','cycle',
 'font','getopt','getopt1','global','graphics','hirespic','id','inv','keyboard',
 'logic','lzw','main','menu','motion','objects','fileglob','op_cmd','op_dbg',
 'op_test','patches','path','picture','picview','rand','savegame','silent',
 'sound','sprite','text','view','words','dummy','nullvid','nothing']

def build(t):
    inp=t/'inputs';inp.mkdir()
    for n in APP_NAMES:
        shutil.copyfile(ROOT/'fixtures/sarien'/('%s.obj'%n),inp/('%s.obj'%n))
    for n,src in [('LIBC.LIB',ROOT/'fixtures/LIBC.LIB'),
                  ('OS2386.LIB',ROOT/'fixtures/OS2386.LIB'),
                  ('sarien.def',ROOT/'fixtures/sarien/sarien.def')]:
        shutil.copyfile(src,inp/n)
    exe=t/'sarien-generic-experimental.exe'
    cmd=[str(ROOT/'link386r4l'),'-o',str(exe),
         '-d',str(inp/'sarien.def'),'--stack=256000']
    cmd += [str(inp/('%s.obj'%n)) for n in APP_NAMES]
    cmd += [str(inp/'LIBC.LIB'),str(inp/'OS2386.LIB')]
    cp=subprocess.run(cmd,capture_output=True,text=True)
    assert cp.returncode==0,(cp.stdout[-1800:],cp.stderr[-1800:])
    assert exe.exists() and exe.stat().st_size==113756
    return exe,cp.stdout

def main():
    with tempfile.TemporaryDirectory(prefix='link386-r4l-') as td:
        t=Path(td)
        first,log=build(t)
        contents=first.read_bytes()
        # Link again in a new independent input folder to test repeatability.
        t2=t/'repeat';t2.mkdir()
        second,_=build(t2)
        assert contents==second.read_bytes(), 'nondeterministic R4L output'
        # Oracle is read only after independent outputs have been completed.
        ref=ROOT/'fixtures/sarien/sarien.exe'
        expected=extract(ref)
        actual=extract(first)
        assert len(expected)==len(actual)==2
        mismatch=[]
        for i in range(2):
            mask=(t/('object%d-r4f-mask.bin'%(i+1))).read_bytes()
            assert len(expected[i])==len(actual[i])==len(mask)
            counts=Counter()
            for a,(one,two) in enumerate(zip(expected[i],actual[i])):
                if one!=two:counts[mask[a]]+=1
            mismatch.append(counts)
        assert mismatch[0][1]==0,('CODE ordinary bytes differ',mismatch[0])
        assert mismatch[0][0]==0,('CODE zero-fill differs',mismatch[0])
        assert mismatch[1][1]==0 and mismatch[1][0]==0 and mismatch[1][2]==0,('DATA mismatches',mismatch[1])
        rr,_=read_fixups(ref)
        aa,_=read_fixups(first)
        matched=sum((rr&aa).values())
        assert sum(rr.values())==2789 and sum(aa.values())==2789
        assert matched==2789 and rr==aa,('relocation regression',matched)
        print('R4L GENERIC ORDER PASS — input-driven, no reference EXE at link time')
        print('R4L code ordinary/unsourced byte mismatches: 0 / 0')
        print('R4L data all mapped byte mismatches: 0')
        print('R4L relocation site/type/target matches: %d / 2789 (0 outstanding)'%matched)
        print('R4L source-only output: %d bytes SHA256=%s'%(len(contents),hashlib.sha256(contents).hexdigest()))
        print('R4L repeat independent rebuild: byte-identical')
        assert 'R4C IMAGES selected=173' in log
        assert 'R4L POLICY OMF_BFS_HASH' in log
        assert hashlib.sha256(contents).hexdigest()=='fd845cf0839bdb2fc77740c3e7e05616cd9eb731fd27b96809562ab4a25ca287', 'not byte-identical to frozen R4K output'

if __name__=='__main__':
    main()
