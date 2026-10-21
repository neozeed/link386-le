#!/usr/bin/env python3
"""R3I exact-feature regression: real 16-bit CODE16, stack, common, MAP sizes."""
import subprocess,re
from pathlib import Path
from r3h_oracle import inspect_map
root=Path(__file__).resolve().parent.parent
obj=['cmd32os2','cmdparse','cmdbatch','cmdfile','cmdos2_os2','cmdos2_session_os2','cmdos2_env']
cmd=[str(root/'link386r3g'),'--stack=61440',*[str(root/'fixtures/cmd32'/(n+'.obj')) for n in obj],str(root/'fixtures/LIBC.LIB'),str(root/'fixtures/OS2386.LIB')]
p=subprocess.run(cmd,capture_output=True,text=True,check=True).stdout
r=inspect_map(str(root/'oracles/cmd32os2_os2.map'))
seg={}
for m in re.finditer(r'OUTSEG \d+ name=(\S+) class=(\S+) use32=(\d+) combine=\d+ align=\d+ offset=[0-9A-F]+ size=(\d+)',p):key=(m[1],m[2],int(m[3]));seg[key]=seg.get(key,0)+int(m[4])
ref={(s['name'],s['cls'],int(s['bits']==32)):s['size'] for s in r['segments']}
for key in [('$C20006_CODE16','CODE',0),('STACK','STACK',1),('c_common','BSS',1),('_BSS','BSS',1)]:
 assert seg.get(key)==ref.get(key),(key,seg.get(key),ref.get(key))
assert 'COMMON ' in p and 'c_common_offset=' in p
bad=subprocess.run([str(root/'link386r3g'),'--stack=invalid',str(root/'fixtures/hi.obj')],capture_output=True,text=True)
assert bad.returncode!=0
print('R3I PASS: 16-bit CODE16, stack size, c_common and BSS match Microsoft CMD32 MAP')
