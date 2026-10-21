#!/usr/bin/env python3
"""R4B evidence: source-dependent, strictly reference-assisted hello LE relinker."""
from pathlib import Path
import hashlib, struct, subprocess, tempfile, sys
from r3s_le_fixups import decode
R = Path(__file__).resolve().parents[1]
EXE = R / 'link386r4b'
REF = R / 'fixtures/hi-reference.exe'
OBJ = R / 'fixtures/hi.obj'
LIBC = R / 'fixtures/LIBC.LIB'
OS2 = R / 'fixtures/OS2386.LIB'

def run(*args, success=True):
    p = subprocess.run([str(EXE),*map(str,args)],capture_output=True,text=True)
    if (p.returncode==0)!=success:
        raise AssertionError(f'wrong return code {p.returncode}: {p.stdout}\n{p.stderr}')
    return p

def mutate_data(b):
    b=bytearray(b);i=0;matches=0
    while i<len(b):
        if i+3>len(b):raise AssertionError('bad input')
        rec=i;t=b[i];n=b[i+1]|(b[i+2]<<8);end=i+3+n
        if not n or end>len(b):raise AssertionError('bad record')
        if t==0xa1:
            p=i+3;idx=b[p];p+=1;off=struct.unpack_from('<I',b,p)[0];p+=4
            if idx==2 and off==0 and b[p:p+5]==b'hi!\n\0':
                b[p:p+5]=b'yo!\n\0'
                b[end-1]=(-sum(b[rec:end-1])) & 255
                matches+=1
        i=end
    assert matches==1
    return bytes(b)

def le_read(b,obj,offset,count):
    h=struct.unpack_from('<I',b,0x3c)[0]
    p=h+struct.unpack_from('<I',b,h+0x40)[0]+24*(obj-1)
    first=struct.unpack_from('<I',b,p+12)[0]
    pagesize=struct.unpack_from('<I',b,h+0x28)[0]
    pm=h+struct.unpack_from('<I',b,h+0x48)[0]+4*(first-1+offset//pagesize)
    physical=int.from_bytes(b[pm:pm+3],'big')
    disk=struct.unpack_from('<I',b,h+0x80)[0]+(physical-1)*pagesize+offset%pagesize
    return b[disk:disk+count]

def main():
    with tempfile.TemporaryDirectory() as td:
        d=Path(td)
        output=d/'new-hi.exe'
        p=run('-r',REF,'-o',output,OBJ,LIBC,OS2)
        assert 'application_fixups=2' in p.stdout and 'inherited_loader_sites=159+12' in p.stdout
        assert output.read_bytes()==REF.read_bytes(), 'unaltered input must reproduce Microsoft executable byte-for-byte'
        dec=decode(output)
        assert len(dec['sites'])==171
        assert len([s for s in dec['sites'] if s[3]==0])==159
        assert len([s for s in dec['sites'] if s[3]!=0])==12
        assert le_read(output.read_bytes(),1,0x10,16)==bytes.fromhex('68 44 00 02 00 e8 4e 05 00 00 83 c4 04 c3 90 90')
        changed=d/'changed.obj'; changed.write_bytes(mutate_data(OBJ.read_bytes()))
        negative=d/'must-not-exist.exe'
        run('-r',REF,'-o',negative,changed,LIBC,OS2,success=False)
        assert not negative.exists()
        run('--allow-data-edit','-r',REF,'-o',negative,changed,LIBC,OS2)
        a=output.read_bytes();b=negative.read_bytes();changed_indexes=[i for i,(v,w) in enumerate(zip(a,b)) if v!=w]
        assert len(b)==len(a) and len(changed_indexes)==2, f'Unexpected changed byte count {len(changed_indexes)}'
        assert le_read(b,2,0x44,5)==b'yo!\n\x00'
        assert le_read(a,2,0x44,5)==b'hi!\n\x00'
        assert decode(negative)['sites']==dec['sites']
        damaged=d/'damaged.exe';x=bytearray(REF.read_bytes());x[-5]^=1;damaged.write_bytes(x)
        run('-r',damaged,'-o',d/'badref.exe',OBJ,LIBC,OS2,success=False)
        badobj=d/'bad.obj';x=bytearray(OBJ.read_bytes());x[-18]^=1;badobj.write_bytes(x)
        run('-r',REF,'-o',d/'badomf.exe',badobj,LIBC,OS2,success=False)
        for name in ('same','changed'):
            f=output if name=='same' else negative
            print(f'{name.upper()} {f.name} bytes={len(f.read_bytes())} md5={hashlib.md5(f.read_bytes()).hexdigest()} sha256={hashlib.sha256(f.read_bytes()).hexdigest()}')
        print('OMF relocation patch sites: 00010011 ABS32 -> 00020044, 00010016 REL32 -> _printf 00010568')
        print('LE page order (logical -> physical): 2,3,1')
        print('Reference runtime relocations verified but inherited: 159 internal + 12 imports')
        print('Negative tests: modified OMF without opt-in, corrupted reference, corrupted OMF checksum PASS')
        print('R4B PASS: reference-assisted executable relink and changed-source proof')
if __name__=='__main__': main()
