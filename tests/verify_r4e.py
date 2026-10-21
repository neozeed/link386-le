#!/usr/bin/env python3
"""R4E post-build oracle: reference EXEs are read only after source-only build.
Target scope: supported C/386 CMD32 console and CMD32 seven-object profiles.
This test deliberately publishes the one known full-CMD32 mismatch rather than
silently patching it with Microsoft reference bytes.
"""
from pathlib import Path
import subprocess, tempfile, struct, hashlib, csv
from r3s_le_fixups import decode
ROOT=Path(__file__).resolve().parents[1]
FIX=ROOT/'fixtures'
B=FIX/'cmd32/shell'
LIBS=[FIX/'LIBC.LIB',FIX/'OS2386.LIB']

def u32(b,p):return struct.unpack_from('<I',b,p)[0]
def image(path):
    b=path.read_bytes();h=u32(b,0x3c);assert b[h:h+2]==b'LE'
    ps=u32(b,h+0x28);n=u32(b,h+0x14);objs=u32(b,h+0x44)
    pt=h+u32(b,h+0x48);ot=h+u32(b,h+0x40);dat=u32(b,h+0x80)
    phys=[int.from_bytes(b[pt+4*i:pt+4*i+3],'big') for i in range(n)]
    assert sorted(phys)==list(range(1,n+1)),phys
    out=[]
    for i in range(objs):
        p=ot+24*i;size,base,first,count=[u32(b,p+k) for k in (0,4,12,16)]
        blob=b''.join(b[dat+(phys[j-1]-1)*ps:dat+phys[j-1]*ps].ljust(ps,b'\x00')
                      for j in range(first,min(first+count,n+1)))
        out.append((size,base,blob))
    return out,phys

def do_link(tmp,which):
    if which=='console':
        objs=[B/f for f in ('cmdos2_os2_console_test.obj','cmdos2_os2.obj','cmdos2_env.obj')]
        deff=B/'cmdos2_os2_console_test.def'
        ref=B/'cmdos2_os2_console_test.exe'
        expect=(0x2f16,0x62,0x22a00);limit=(0x2f16,0x62,0x734)
        imports=['DOSCALLS','VIOCALLS','KBDCALLS'];stack=[];entry=0x1718
        expected_diff=[[],[],[]]
    else:
        objs=[B/f for f in ('cmd32os2.obj','cmdparse.obj','cmdbatch.obj','cmdfile.obj','cmdos2_os2.obj','cmdos2_session_os2.obj','cmdos2_env.obj')]
        deff=B/'cmd32os2_os2.def';ref=B/'cmd32os2_os2.exe'
        expect=(0xf1d1,0x62,0x51990);limit=(0xf1d1,0x62,0x3068)
        imports=['DOSCALLS','VIOCALLS','KBDCALLS','SESMGR'];stack=['--stack=61440'];entry=0xb9f4
        expected_diff=[[],[],[]]
    out=tmp/f'{which}-R4E.exe'
    cmd=[str(ROOT/'link386r4e'),'-o',str(out),'-d',str(deff),*stack,*map(str,objs),*map(str,LIBS)]
    r=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
    assert r.returncode==0,(r.stdout[-700:],r.stderr)
    assert out.exists() and out.stat().st_size>1000
    return out,ref,expect,limit,imports,entry,expected_diff,cmd

def compare(out,ref,expect,limit,imports,entry,expected_diff):
    b=out.read_bytes();h=u32(b,0x3c)
    assert b[:2]==b'MZ' and b[h:h+2]==b'LE'
    assert struct.unpack_from('<HH',b,h+8)==(2,1)
    assert u32(b,h+0x18)==1 and u32(b,h+0x1c)==entry
    a,phys=image(out);r,refphys=image(ref)
    assert len(a)==len(r)==3
    assert [x[0] for x in a]==list(expect)
    assert [x[1] for x in a]==[0x10000,0x20000,0x30000]
    assert phys==refphys
    dd=decode(out)
    assert dd['modules']==imports
    assert len(dd['sites'])>=200
    assert sum(x[3]==1 for x in dd['sites'])>40
    # Mask precisely the generated, deferred DLL source operand sites. The
    # source mask is NOT used for any expected non-import differences.
    page_to_log={physical:logical for logical,physical in enumerate(phys,1)}
    sources={}
    for page,offset,kind,mode,name,target in dd['sites']:
        assert -4096<=offset<4096
        logical=page_to_log[page]
        if logical<=((expect[0]+4095)//4096):obj=0;local=(logical-1)*4096+offset
        elif logical==((expect[0]+4095)//4096)+1:obj=1;local=offset
        else:obj=2;local=(logical-((expect[0]+4095)//4096)-2)*4096+offset
        if mode==1:
            width={8:4,3:4}.get(kind,4)
            sources.setdefault(obj,set()).update(range(local,local+width))
    mismatches=[]
    for obj,((size,base,body),(rsize,rbase,other),end) in enumerate(zip(a,r,limit)):
        assert size==rsize and base==rbase
        assert end<=len(body) and end<=len(other)
        bad=[p for p in range(end) if p not in sources.get(obj,set()) and body[p]!=other[p]]
        assert bad==expected_diff[obj],(obj,len(bad),bad[:30])
        mismatches.append(len(bad))
    # The R4E executable must be reproducible from its OMF, independent of its oracle.
    return dd,mismatches

def main():
    with tempfile.TemporaryDirectory(prefix='r4e-link-') as t:
        root=Path(t)
        for which in ('console','full'):
            place=root/which;place.mkdir()
            out,ref,expect,limit,imports,entry,diff,cmd=do_link(place,which)
            data,mismatch=compare(out,ref,expect,limit,imports,entry,diff)
            second=place/'repeat';second.mkdir()
            out2,*_=do_link(second,which)
            assert out.read_bytes()==out2.read_bytes(),f'{which}: nondeterministic'
            print(f'R4E {which.upper()} PASS output={out.stat().st_size} bytes entry=object1+{entry:04X} '
                  f'pages={len(data["pages"])} fixup_sites={len(data["sites"])} '
                  f'DLLs={",".join(imports)} nonimport_mismatches={mismatch} '
                  f'sha256={hashlib.sha256(out.read_bytes()).hexdigest()}')
        # Negative case: corrupt the OMF record checksum and verify failure.
        bad=root/'invalid.obj';bb=bytearray((B/'cmdos2_os2_console_test.obj').read_bytes());bb[20]^=1;bad.write_bytes(bb)
        invalid=root/'broken.exe'
        cmd=[str(ROOT/'link386r4e'),'-o',str(invalid),'-d',str(B/'cmdos2_os2_console_test.def'),
             str(bad),str(B/'cmdos2_os2.obj'),str(B/'cmdos2_env.obj'),*map(str,LIBS)]
        check=subprocess.run(cmd,capture_output=True)
        assert check.returncode!=0 and not invalid.exists()
        print('R4E REJECT BAD OMF PASS; reference EXEs never used as input to linker')

if __name__=='__main__':main()
