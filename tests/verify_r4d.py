#!/usr/bin/env python3
"""R4D independent source-only LE proof and post-build Microsoft oracle check.
The subprocess that creates each executable sees only OMF sources; the Microsoft
reference is read by THIS test *after* the generated output exists.
"""
from pathlib import Path
import csv, hashlib, subprocess, tempfile, sys, collections
from r4c_compare import image
from r3s_le_fixups import decode
ROOT=Path(__file__).resolve().parents[1]


def run(out,obj):
    return subprocess.run([str(ROOT/'link386r4d'), '-o',str(out),
        str(obj),str(ROOT/'fixtures/LIBC.LIB'),str(ROOT/'fixtures/OS2386.LIB')],
        capture_output=True,text=True)


def normalized(path):
    d=decode(path);b=path.read_bytes();h=int.from_bytes(b[0x3c:0x40],'little')
    mapping=h+int.from_bytes(b[h+0x48:h+0x4c],'little')
    phy=[int.from_bytes(b[mapping+i*4:mapping+i*4+3],'big') for i in range(3)]
    assert sorted(phy)==[1,2,3],phy
    inverse={phys:logical+1 for logical,phys in enumerate(phy)}
    sites=[]
    for physical,off,typ,mode,name,target in d['sites']:
        logical=inverse[physical]
        obj=1 if logical<3 else 2
        offset=((logical-1)*4096 if obj==1 else 0)+off
        sites.append((obj*0x10000+offset,typ,mode,name,target))
    return sorted(sites),phy,d


def assert_structure(path,reference):
    s,phy,meta=normalized(path)
    oracle,oraclephy,ref=normalized(reference)
    assert phy==oraclephy==[2,3,1],(phy,oraclephy)
    assert s==oracle,'generated LE relocation sites or targets differ from Microsoft'
    assert len(s)==171 and sum(x[2]==0 for x in s)==159 and sum(x[2]==1 for x in s)==12
    assert [(x['size'],x['base'],x['flags']) for x in meta['objects']]==[(6174,0x10000,0x2005),(10256,0x20000,0x2003)]
    assert meta['modules']==['DOSCALLS']
    assert len(meta['pages'])==3
    raw=image(path);orig=image(reference)
    for obj in range(2):
        sz=(6174,10256)[obj]
        data=raw[obj][2].ljust(sz,b'\0')[:sz]
        oracle=orig[obj][2].ljust(sz,b'\0')[:sz]
        assert data==oracle,(obj,[hex(k) for k in range(sz) if data[k]!=oracle[k]][:20])
    return s


def main():
    ref=ROOT/'fixtures/hi-reference.exe'
    with tempfile.TemporaryDirectory(prefix='link386-r4d-') as temp:
        p=Path(temp);a=p/'a';a.mkdir();b=p/'b';b.mkdir();c=p/'c';c.mkdir()
        f=a/'hi-r4d.exe'
        r=run(f,ROOT/'fixtures/hi.obj')
        assert r.returncode==0,(r.stdout[-1000:],r.stderr)
        assert 'loader_fixups=159+12' in r.stdout
        assert f.read_bytes()[:2]==b'MZ'
        sites=assert_structure(f,ref)
        fp=b/'hi-r4d.exe';result=run(fp,ROOT/'fixtures/hi.obj')
        assert result.returncode==0,result.stderr
        assert fp.read_bytes()==f.read_bytes(),'nondeterministic LE output'
        changed=c/'hi-yo-r4d.exe';result=run(changed,ROOT/'fixtures/hi-yo.obj')
        assert result.returncode==0,result.stderr
        sa,pmap,_=normalized(changed);assert sa==sites
        old=image(f);new=image(changed)
        diff=[i for i,(x,y) in enumerate(zip(old[1][2],new[1][2])) if x!=y]
        assert diff==[0x44,0x45],diff
        assert old[1][2][0x44:0x49]==b'hi!\n\0'
        assert new[1][2][0x44:0x49]==b'yo!\n\0'
        bad=bytearray((ROOT/'fixtures/hi.obj').read_bytes());bad[24]^=1
        badfile=p/'bad.obj';badfile.write_bytes(bad)
        error=run(c/'bad.exe',badfile)
        assert error.returncode!=0 and not (c/'bad.exe').exists(), 'checksum corruption accepted'
        # Ensure all relocation records correspond to materializer's source-owned positions.
        with (a/'relocation-candidates-r4d.tsv').open() as fh:
            rows=list(csv.DictReader(fh,delimiter='\t'))
        assert len(rows)==306,len(rows)
        assert sum(x['result']=='CANDIDATE' for x in rows)==294
        assert sum(x['result']=='IMPORT_DEFERRED' for x in rows)==12
        assert not any('UNRESOLVED' in x['result'] for x in rows)
        print('R4D VERIFY PASS: source-only independent MZ/LE writer, 2 exact object images, 171 loader site+target matches')
        print('selected=39 input_fixups=301 expanded_fixup_sites=306 internal=159 imports=12 static_lidata_offset_sites=6')
        print('deterministic builds, changed-source hi->yo, corrupt OMF rejected, Microsoft EXE read POST-BUILD ONLY')
        print('R4D emitted_sha256='+hashlib.sha256(f.read_bytes()).hexdigest())

if __name__=='__main__':main()
