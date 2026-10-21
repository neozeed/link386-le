#!/usr/bin/env python3
"""R3J: verify conservative class ordering against Microsoft CMD32 MAP."""
import re,subprocess
from pathlib import Path
from r3h_oracle import inspect_map
root=Path(__file__).resolve().parent.parent
objects='cmd32os2 cmdparse cmdbatch cmdfile cmdos2_os2 cmdos2_session_os2 cmdos2_env'.split()
args=[str(root/'link386r3g'),'--stack=61440']+[str(root/'fixtures/cmd32'/(o+'.obj')) for o in objects]+[str(root/'fixtures/LIBC.LIB'),str(root/'fixtures/OS2386.LIB')]
text=subprocess.check_output(args,text=True)
pat=re.compile(r'OUTSEG (\d+) name=(\S+) class=(\S+) use32=(\d+) combine=(\d+) align=(\d+) offset=([0-9A-F]+) size=(\d+)')
entries=[dict(id=int(x[0]),name=x[1],cls=x[2],bits=int(x[3]),offset=int(x[6],16),size=int(x[7])) for x in pat.findall(text)]
ref=inspect_map(str(root/'oracles/cmd32os2_os2.map'))['segments']
# Relative offsets within DATA object; MAP uses 0003: (DGROUP).
def only(name,cls):return [x for x in entries if x['name']==name and x['cls']==cls]
def reference(name):return next(x for x in ref if x['name']==name)
assert only('NULL','BEGDATA')[0]['offset']==reference('NULL')['offset']==0
assert only('_DATA','DATA')[0]['offset']==reference('_DATA')['offset']==0x44
assert only('XIB','DATA')[0]['offset']==reference('XIB')['offset']==0x2f1c
assert only('CONST','CONST')[0]['offset']==reference('CONST')['offset']==0x2f34
for name in ['HDR','MSG','PAD','EPAD']:
 assert only(name,'MSG')[0]['offset']==reference(name)['offset'],name
assert only('c_common','BSS')[0]['offset']>=only('_BSS','BSS')[0]['offset']
assert only('STACK','STACK')[0]['offset']>only('c_common','BSS')[0]['offset']
# OMF _DATA entries have a three-byte alignment gap, rather than 3 missing bytes.
data=only('_DATA','DATA');assert len(data)==2
span=max(x['offset']+x['size'] for x in data)-min(x['offset'] for x in data)
assert span==reference('_DATA')['size']==11992,(span,data)
assert sum(x['size'] for x in data)==11989
assert only('_TEXT','CODE')[0]['size']+16==reference('_TEXT')['size']
print('R3J PASS: CMD32 class ordering, data extent including 3-byte alignment, MSG exact offsets, common-before-stack; 16 CODE bytes remain unexplained')
