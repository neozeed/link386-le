#!/usr/bin/env python3
import subprocess
from pathlib import Path
root=Path(__file__).resolve().parent.parent
exe=root/'link386r2'

def plan(args):
    x=subprocess.run([str(exe),*map(str,args)],capture_output=True,text=True,timeout=120)
    assert x.returncode==0,(x.stderr[-500:],x.returncode)
    return x.stdout

hi=plan([root/'fixtures/hi.obj',root/'fixtures/LIBC.LIB',root/'fixtures/OS2386.LIB'])
assert 'MODULE crt0.asm' in hi
assert 'MODULE printf.c' in hi
assert 'EXT _printf resolved' in hi
assert 'SUMMARY selected_modules=39' in hi
assert 'fixups=301' in hi
cmd=plan([*sorted((root/'fixtures/cmd32').glob('*.obj')),root/'fixtures/LIBC.LIB',root/'fixtures/OS2386.LIB'])
assert 'SUMMARY selected_modules=123' in cmd
assert 'fixups=2710' in cmd
print('R3C PASS: LIBC.LIB LIDATA32 parsing and FIXUPP32 association; hi + 8 CMD32 objects')
