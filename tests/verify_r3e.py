#!/usr/bin/env python3
import subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
def run(xs):
 p=subprocess.run([str(root/'link386r2')]+[str(x) for x in xs],capture_output=True,text=True)
 assert p.returncode==0,p.stderr
 assert 'unresolved=0' in p.stdout, p.stdout[-600:]
 assert '_edata RESOLVED' in p.stdout and '_end   RESOLVED' in p.stdout
 assert 'VA=DEFERRED' in p.stdout
 return p.stdout
f=root/'fixtures'
hi=run([f/'hi.obj',f/'LIBC.LIB',f/'OS2386.LIB'])
sa=run(sorted((f/'sarien').glob('*.obj'))+[f/'LIBC.LIB',f/'OS2386.LIB'])
assert 'selected_modules=39' in hi
assert 'selected_modules=172' in sa
print('R3E hello/Sarien synthetic-boundary symbol tests PASS')
