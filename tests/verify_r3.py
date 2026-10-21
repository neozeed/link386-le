#!/usr/bin/env python3
"""Independent bounded R3 structural oracle, with R2 regression."""
import pathlib, subprocess, struct, tempfile, hashlib
root=pathlib.Path(__file__).resolve().parents[1]
def rec(t,b):
    n=len(b)+1
    z=bytes([t,n&255,n>>8])+b
    return z+bytes([(-sum(z))&255])
def ix(i): return bytes([i])
def s(t): b=t.encode('ascii');return bytes([len(b)])+b
# THEADR, LNAMES, SEGDEF32, PUBDEF32, LEDATA32, MODEND32
sample=b''.join([rec(0x80,s('tiny')),rec(0x96,s('_TEXT')+s('CODE')+s('FLAT')),rec(0x99,bytes([0xa1])+struct.pack('<I',1)+ix(1)+ix(2)+ix(1)),rec(0x91,ix(0)+ix(1)+s('_start')+struct.pack('<I',0)+ix(0)),rec(0xa1,ix(1)+struct.pack('<I',0)+b'\xC3'),rec(0x8b,b'\x00')])
def u32(b,p):return struct.unpack_from('<I',b,p)[0]
with tempfile.TemporaryDirectory() as tmp:
    obj=pathlib.Path(tmp)/'tiny.obj'; exe=pathlib.Path(tmp)/'tiny.exe'; obj.write_bytes(sample)
    q=subprocess.run([str(root/'link386r3'),'-o',str(exe),'-e','_start',str(obj)],capture_output=True,text=True)
    assert q.returncode==0,(q.stdout,q.stderr)
    b=exe.read_bytes();base=u32(b,0x3c);h=base
    assert b[:2]==b'MZ' and b[h:h+2]==b'LE'
    assert u32(b,h+0x14)==1 and u32(b,h+0x44)==2
    ot=u32(b,h+0x40);om=u32(b,h+0x48)
    assert u32(b,h+0x18)==1 and u32(b,h+0x1c)==0
    assert u32(b,h+ot)==1 and u32(b,h+ot+4)==0x10000
    assert u32(b,h+ot+16)==1 and u32(b,h+ot+24)==0x10000
    assert b[h+om:h+om+4]==b'\x00\x00\x01\x00'
    data=u32(b,h+0x80)
    assert data%512==0 and b[data]==0xc3 and len(b)==data+4096
    ft=u32(b,h+0x68);fr=u32(b,h+0x6c)
    assert fr-ft==8 and b[h+ft:h+fr]==b'\0'*8
    checksum=hashlib.sha256(b).hexdigest()
    print('R3 synthetic OMF32 -> MZ/LE PASS',len(b),'bytes sha256',checksum)
    bad=subprocess.run([str(root/'link386r3'),'-o',str(pathlib.Path(tmp)/'hi.exe'),'-e','_main',str(root/'fixtures/hi.obj')],capture_output=True,text=True)
    assert bad.returncode!=0 and 'unresolved external' in bad.stderr
    assert not (pathlib.Path(tmp)/'hi.exe').exists()
    print('R3 missing C/386 runtime rejection PASS')
subprocess.run(['python3',str(root/'tests/verify.py')],cwd=root,check=True)
print('R2 regressions PASS')
