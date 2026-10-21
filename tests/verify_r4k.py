#!/usr/bin/env python3
"""R4K: rebuild only from OMF+LIB inputs; independently compare against Microsoft LE.
The reference executable is never passed to the linker or copied into the link's input directory.
"""
from pathlib import Path
from collections import Counter
import contextlib
import io
import subprocess
import shutil
import tempfile
import hashlib
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tests'))
from verify_r4e_fixups import read_fixups
with contextlib.redirect_stdout(io.StringIO()):
    from r4h_compare import extract
APP_NAMES=['agi','agi_v2','agi_v3','agi_v4','checks','cli','console','cycle',
 'font','getopt','getopt1','global','graphics','hirespic','id','inv','keyboard',
 'logic','lzw','main','menu','motion','objects','fileglob','op_cmd','op_dbg',
 'op_test','patches','path','picture','picview','rand','savegame','silent',
 'sound','sprite','text','view','words','dummy','nullvid','nothing']
def digest(data): return hashlib.sha256(data).hexdigest()
def verify():
    ref=ROOT/'fixtures/sarien/sarien.exe'
    assert ref.exists(), 'missing reference EXE for post-build comparison'
    with tempfile.TemporaryDirectory(prefix='link386-r4k-') as td:
        t=Path(td);inputdir=t/'input';inputdir.mkdir()
        for name in APP_NAMES:
            shutil.copyfile(ROOT/('fixtures/sarien/%s.obj'%name),inputdir/(name+'.obj'))
        for name,where in [('sarien.def','fixtures/sarien/sarien.def'),('LIBC.LIB','fixtures/LIBC.LIB'),('OS2386.LIB','fixtures/OS2386.LIB')]:
            shutil.copyfile(ROOT/where,inputdir/name)
        base=[str(ROOT/'link386r4e'),'--r4k-evidence-order','-d',str(inputdir/'sarien.def'),
              '--stack=256000']
        base.extend(str(inputdir/(name+'.obj')) for name in APP_NAMES)
        base.extend([str(inputdir/'LIBC.LIB'),str(inputdir/'OS2386.LIB')])
        out=t/'sarien-r4k.exe'
        cp=subprocess.run(base[:2]+['-o',str(out)]+base[2:],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        assert cp.returncode==0,cp.stdout[-1000:]+'\n'+cp.stderr[-1000:]
        first=out.read_bytes()
        assert first[:2]==b'MZ'
        cp=subprocess.run(base[:2]+['-o',str(out)]+base[2:],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        assert cp.returncode==0,cp.stderr[-1000:]
        assert out.read_bytes()==first,'nondeterministic link'
        rr,_=read_fixups(ref)
        aa,_=read_fixups(out)
        assert rr==aa, 'fixup sites/target mismatch: extra=%d missing=%d'%(sum((aa-rr).values()),sum((rr-aa).values()))
        ms=extract(ref);ours=extract(out)
        assert len(ms)==2 and len(ours)==2
        for i in range(2):
            assert ms[i]==ours[i], 'mapped object %d differs at %s'%(
                i+1, next((hex(k) for k,(a,b) in enumerate(zip(ms[i],ours[i])) if a!=b),'end'))
        assert len(first)==113756,'unexpected output size'
        print('R4K PASS: source-only reconstruction with reference-derived CODE/DATA/COMMON ordering')
        print('Microsoft mapped images: object1=%d object2=%d; exactly identical'%(len(ms[0]),len(ms[1])))
        print('Loader fixup sites and targets: %d exact, 0 missing, 0 extra'%sum(rr.values()))
        print('Repeated build byte-identical; REF EXE not present in linker input folder')
        print('sarien-r4k.exe SHA256 '+digest(first))
if __name__=='__main__':
    try:verify()
    except Exception as ex:
        print('R4K FAIL:',ex,file=sys.stderr)
        raise
