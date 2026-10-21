#!/usr/bin/env python3
"""R3L isolated GROUP identity regression and real CMD32 coverage."""
from pathlib import Path
import subprocess,tempfile,re
root=Path(__file__).resolve().parent.parent
exe=root/'link386r3g'
def rec(t,b):
    n=len(b)+1
    raw=bytes((t,n&255,n>>8))+b
    return raw+bytes((-sum(raw))&255 for _ in range(1))
def name(s):
    v=s.encode('ascii');return bytes([len(v)])+v
def obj(module,group):
    # LNAMES: segment, class, group; SEGDEF32 ACBP alignment=paragraph, combine=PUBLIC, use32
    b=rec(0x80,name(module))
    b+=rec(0x96,name('SAME')+name('DATA')+name(group))
    b+=rec(0x99,bytes([0x69,4,0,0,0,1,2,0]))
    b+=rec(0x9a,bytes([3,0xff,1]))
    b+=rec(0x8b,bytes([0]))
    return b
with tempfile.TemporaryDirectory() as d:
    a,b=Path(d)/'a.obj',Path(d)/'b.obj'
    a.write_bytes(obj('A','ALPHA'));b.write_bytes(obj('B','BETA'))
    t=subprocess.check_output([str(exe),str(a),str(b)],text=True)
    assert 'group=ALPHA' in t and 'group=BETA' in t,t
    assert 'combined_segments=2' in t,t
    b.write_bytes(obj('B','ALPHA'))
    t=subprocess.check_output([str(exe),str(a),str(b)],text=True)
    assert 'combined_segments=1' in t,t
cmd='cmd32os2 cmdparse cmdbatch cmdfile cmdos2_os2 cmdos2_session_os2 cmdos2_env'.split()
args=[str(exe),'--stack=61440']+[str(root/'fixtures/cmd32'/(n+'.obj')) for n in cmd]+[str(root/'fixtures/LIBC.LIB'),str(root/'fixtures/OS2386.LIB')]
t=subprocess.check_output(args,text=True)
assert 'GROUP-MEMBER group=DGROUP' in t
assert re.search(r'groups=1\s*$',t)
assert '_TEXT' in t
print('R3L PASS: distinct GROUPs never merge; same-named GROUPs merge; CMD32 DGROUP explicitly identified')
