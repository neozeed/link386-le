#!/usr/bin/env python3
"""R3V verified code-import shift, real LE and source fingerprints; no writebacks."""
from pathlib import Path
import re,sys
from r3t_correlate import analyze
from r3v_contribution_fingerprints import scan
ROOT=Path(__file__).resolve().parents[1]
R=re.compile(r'^R3N IMPORT module=(\S+) site=([0-9a-fA-F]+) loc=\d+ symbol=(\S+)')
names={'DOS32QUERYSYSINFO':348,'DOS32QUERYHTYPE':224,'DOS32EXIT':234,'DOS32WRITE':282,'DOS32SETFILEPTR':256,'DOS32ALLOCMEM':299,'DOS32SETMEM':305,'DOS32FREEMEM':304}
d,actual,*_=analyze(ROOT/'fixtures/hi-reference.exe',ROOT/'r3n-hi-report.txt')
a={}
for va,fixes in actual.items():
 for loc,mode,name,num in fixes:
  if mode: a.setdefault(num,[]).append(va)
pred=[]
for line in (ROOT/'r3n-hi-report.txt').read_text().splitlines():
 m=R.match(line)
 if m:pred.append((m.group(1),int(m.group(2),16),m.group(3),names[m.group(3)]))
assert len(pred)==12 and sum(len(v) for v in a.values())==12
print('R3V IMPORT RECONCILIATION — REFERENCE READ ONLY')
print('CODE IMPORTS: eight exact shifted site matches')
matched=[]
for mod,va,name,ordinal in pred:
 if name.startswith(('DOS32ALLOCMEM','DOS32SETMEM','DOS32FREEMEM')):continue
 wanted=va+0x1010
 assert wanted in a[ordinal],(mod,name,hex(wanted),a[ordinal])
 matched.append(wanted)
 print(' VERIFIED code module=%s import=%s original=%08X actual=%08X shift=+1010'%(mod,name,va,wanted))
assert len(matched)==8 and len(set(matched))==8
print('DATA IMPORTS: four loader sites (mapping to source contributions not yet verified)')
for ordinal in (299,305,304):
 for va in a[ordinal]:
  assert 0x20000<=va<0x23000
  print(' DEFERRED DOSCALLS.%d object=2 site=%08X'%(ordinal,va))
print('Fingerprint evidence follows:')
by=scan()
# source code presence proved by immutable LEDATA windows, independent of linker candidate positions
for mod,expected in [('crt0.asm',0x11020),('crt0dat.asm',0x110f8),('nmsghdr.asm',0x11448),('lseek.asm',0x11d3c),('write.asm',0x11d84)]:
 found={x for (s,o),base in by[mod].items() if s=='_TEXT' and o==1 for x in base}
 assert expected in found,(mod,found)
print('PASS: 8/8 code import sites and 5 code contribution fingerprints; 4 data import mappings deferred; no writebacks')
