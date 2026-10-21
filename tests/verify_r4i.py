#!/usr/bin/env python3
"""R4I acceptance regression for independently established contribution order."""
from pathlib import Path
import subprocess,sys
root=Path(__file__).resolve().parents[1]
p=subprocess.run([sys.executable,str(root/'tests/r4i_order.py')],cwd=root,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,check=True)
s=p.stdout
assert 'strong placements 95 of 121' in s
for module,offset in [('fclose.c','0F740'),('fread.asm','0F888'),('fwrite.asm','0F95C'),('malloc.c','0FB40'),('realloc.c','0FD78'),('stdargv.asm','107FC'),('output.asm','10EB4')]:
 assert any('module='+module+' ' in x and 'reference='+offset in x for x in s.splitlines()),module
assert 'module=crt0msg.asm seg=MSG' in s
assert 'UNRESOLVED' in s
assert (root/'build/sarien-r4g.exe').exists()
print('R4I PASS: 95 strong independently ranked placement anchors; unresolved sites retained')
