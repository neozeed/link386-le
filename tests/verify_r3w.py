#!/usr/bin/env python3
"""R3W: immutable OMF <-> LE object ownership evidence, no relocation writebacks."""
from pathlib import Path
from r3s_le_fixups import decode
from r3t_correlate import analyze
from r3v_contribution_fingerprints import scan
from r3p_raw_audit import members
import re
ROOT=Path(__file__).resolve().parents[1]
_,actual,*_=analyze(ROOT/'fixtures/hi-reference.exe',ROOT/'r3n-hi-report.txt')
d=decode(ROOT/'fixtures/hi-reference.exe')
imports={}
for addr,fixes in actual.items():
 for loc,mode,name,ordn in fixes:
  if mode:imports[addr]=ordn
by=scan()
def proof(module,segment,obj,base):
 entries=by[module].get((segment,obj),set())
 assert base in entries,(module,segment,obj,hex(base),entries)

code=[(0x11020,'crt0.asm'),(0x110f8,'crt0dat.asm'),(0x11448,'nmsghdr.asm'),(0x11d3c,'lseek.asm'),(0x11d84,'write.asm')]
for base,module in code:proof(module,'_TEXT',1,base)
proof('heapgrow.c','_TEXT',2,0x202a0)
proof('heapinit.c','_TEXT',2,0x204e4)
assert imports == {0x1104a:348,0x1110f:224,0x111e3:234,0x1149a:282,0x11d6b:256,0x11db3:256,0x11e65:282,0x11efc:282,0x20374:299,0x20456:305,0x204ae:304,0x204ff:299}
assert d['objects'][0]['base']==0x10000 and d['objects'][1]['base']==0x20000
print('R3W OMF SEGMENT vs LE OBJECT OWNERSHIP -- READ ONLY')
print('Five fingerprint-confirmed _TEXT code bases in LE object 1')
for base,module in code:print('  %s _TEXT => LE object 1 base %08X'%(module,base))
print('Two fingerprint-confirmed _TEXT code bases in LE object 2')
for module,base in [('heapgrow.c',0x202a0),('heapinit.c',0x204e4)]:print('  %s _TEXT => LE object 2 base %08X'%(module,base))
print('Data-object import sites (module association based on matching import ordinals and source-code fingerprint proximity; NOT a proven OMF-fixup-to-byte mapping):')
for address in sorted(a for a in imports if a>=0x20000):
 near='heapgrow.c' if address<0x204e4 else 'heapinit.c'
 base=0x202a0 if near=='heapgrow.c' else 0x204e4
 print('  %08X DOSCALLS.%d nearest anchored module=%s offset_from_anchor=%04X'%(address,imports[address],near,address-base))
print('16/32 inference: _TEXT is a logical OMF name, NOT a guarantee of LE executable object membership. Code and data classification must respect grouping and final object mapping.')
print('0x1010 explanation remains HYPOTHESIS: placement of first 0x1000 page + 0x10 alignment; no exact attribution to specific OMF segment or linker synthesis yet.')
print('No relocation writes, no new LE image. PASS')
