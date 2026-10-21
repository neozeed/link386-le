#!/usr/bin/env python3
"""R4D-FIX1: guard machine/OS header identity using a known-good LE oracle.

The prior R4D suite compared images and fixups, but never checked e32_cpu.
The original emitter swapped the integer encodings before writing little endian.
"""
from pathlib import Path
import struct
import subprocess
import tempfile
import hashlib
ROOT=Path(__file__).resolve().parents[1]

def le_header(path):
    b=path.read_bytes()
    assert b[:2]==b'MZ' and len(b)>0x100
    h=struct.unpack_from('<I',b,0x3c)[0]
    assert 0<h<=len(b)-0xb0 and b[h:h+2]==b'LE'
    return b,h

def check(path):
    b,h=le_header(path)
    cpu,os=struct.unpack_from('<HH',b,h+8)
    assert cpu==2,(path,'unexpected LE CPU',hex(cpu))
    assert os==1,(path,'unexpected LE OS',hex(os))
    assert struct.unpack_from('<IIIII',b,h+0x14)==(3,1,0x20,2,0x2810)
    assert struct.unpack_from('<I',b,h+0x28)[0]==4096
    assert struct.unpack_from('<I',b,h+0x44)[0]==2
    assert [int.from_bytes(b[h+0xf4+i*4:h+0xf4+i*4+3],'big') for i in range(3)]==[2,3,1]
    assert struct.unpack_from('<I',b,h+0x74)[0]==1
    return b,h

def build(path,obj):
    cmd=[str(ROOT/'link386r4d'),'-o',str(path),str(obj),str(ROOT/'fixtures/LIBC.LIB'),str(ROOT/'fixtures/OS2386.LIB')]
    result=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
    assert result.returncode==0,(cmd,result.stdout,result.stderr)
    return check(path)

def main():
    reference,h=le_header(ROOT/'fixtures/hi-reference.exe')
    assert struct.unpack_from('<HH',reference,h+8)==(2,1)
    with tempfile.TemporaryDirectory(prefix='link386-r4d-fix1-') as d:
        td=Path(d);a=td/'a';a.mkdir();b=td/'b';b.mkdir()
        original,off=build(a/'hi.exe',ROOT/'fixtures/hi.obj')
        altered,_=build(b/'hi-yo.exe',ROOT/'fixtures/hi-yo.obj')
        assert original!=altered
        assert len(original)==len(altered)==12318
        assert len(original)==len(ROOT.joinpath('build/r4d/hi-r4d-source-only.exe').read_bytes())
        assert original==ROOT.joinpath('build/r4d/hi-r4d-source-only.exe').read_bytes()
        assert altered==ROOT.joinpath('build/r4d/hi-yo-r4d-source-only.exe').read_bytes()
        # Negative regression: old broken R4D LE cpu/os values must fail.
        corrupt=bytearray(original)
        struct.pack_into('<HH',corrupt,off+8,0x200,0x100)
        t=b/'old-header.exe';t.write_bytes(corrupt)
        try: check(t)
        except AssertionError: pass
        else: raise AssertionError('old R4D swapped header was accepted')
        print('R4D-FIX1 HEADER PASS: LE CPU=i386(0x0002) OS=OS/2(0x0001) original/yo source-only')
        print('file length=12318 bytes; object bases and page map [2,3,1] verified')
        print('hi.sha256='+hashlib.sha256(original).hexdigest())
        print('yo.sha256='+hashlib.sha256(altered).hexdigest())
if __name__=='__main__':main()
