#!/usr/bin/env python3
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parent.parent
exe=root/'link386r2'
def plan(paths):
    p=subprocess.run([str(exe),*map(str,paths)],capture_output=True,text=True,timeout=120)
    assert p.returncode == 0,(p.returncode,p.stderr[-500:])
    return p.stdout
libs=[root/'fixtures/LIBC.LIB',root/'fixtures/OS2386.LIB']
sarien=sorted((root/'fixtures/sarien').glob('*.obj'))
assert len(sarien)==42
out=plan(sarien+libs)
assert 'selected_modules=172' in out
assert 'fixups=4425' in out
assert 'unresolved=0' in out
for name,size in [('_game',55324),('_Bitmap',64000),('_opt',48),('_snd',4)]:
    assert f'COMDEF {name} size={size}' in out
    assert f'COMMON {name}' in out
    assert f'EXT {name} resolved' in out
assert 'COMMON SUMMARY globals=' in out
assert 'EXT _edata resolved' in out and 'EXT _end resolved' in out
hi=plan([root/'fixtures/hi.obj']+libs)
assert 'EXT __heap_regions resolved' in hi
assert 'unresolved=0' in hi
print('R3D regression PASS with R3E boundary resolution: 42 Sarien objects, COMDEF BSS plan')
