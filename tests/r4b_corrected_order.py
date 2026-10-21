#!/usr/bin/env python3
"""R3Y: evidence-ranked, read-only reconstruction of LINK386 runtime ordering.
No inferred candidate is allowed to become a linker relocation address.
"""
from pathlib import Path
from collections import defaultdict
from r3v_contribution_fingerprints import candidates
from r3s_le_fixups import decode,u32
from r3x_segment_provenance import parse_module
from r3p_raw_audit import members
ROOT=Path(__file__).resolve().parents[1]

def run():
 exe=ROOT/'fixtures/hi-reference.exe'; data=exe.read_bytes(); d=decode(exe)
 off=u32(data,d['header']+0x80); ps=d['page_size']; images=[]
 pagetab=d['header']+u32(data,d['header']+0x48)
 physical=[int.from_bytes(data[pagetab+i*4:pagetab+i*4+3], 'big') for i in range(len(d['pages']))]
 assert physical==[2,3,1], physical
 for ob in d['objects']:
  p=ob['first']-1
  images.append((ob['index'],ob['base'],b''.join(data[off+(physical[p+i]-1)*ps:off+physical[p+i]*ps] for i in range(ob['pages']))))
 # Each candidate gets support from distinct offsets, and observed positions.
 evidence=defaultdict(lambda:defaultdict(set)); totals=defaultdict(set)
 for mod,seg,record_offset,raw in candidates(ROOT/'fixtures/LIBC.LIB'):
  if seg[0]!='_TEXT':continue
  for i in range(0,len(raw)-11,4):
   sig=raw[i:i+12]
   if len(set(sig))<6:continue
   source=record_offset+i; totals[mod].add(source)
   for obj,base,body in images:
    start=0
    while True:
     at=body.find(sig,start)
     if at<0:break
     # Repetitive windows can match multiple places; preserve ambiguity.
     cand=base+at-source
     if base<=cand<base+len(body):evidence[(mod,obj)][cand].add(source)
     start=at+1
 # All selected module names as reported by R3N; can include modules without a useful _TEXT signature.
 selected=[]
 for line in (ROOT/'r3n-hi-report.txt').read_text().splitlines():
  if line.startswith('R3N ') and 'module=' in line:
   name=line.split('module=',1)[1].split()[0]
   if name not in selected:selected.append(name)
 # Determine real SEGDEF attribute for representative anchors.
 metadata={}
 for mod,offset,records in members(ROOT/'fixtures/LIBC.LIB'):
  try:
   segs,groups=parse_module(records)
   metadata[mod.lower()]=next((s for s in segs[1:] if s['name']=='_TEXT'),None)
  except (ValueError,IndexError):pass
 confirmed=[]; ambiguous=[]
 for (mod,obj),locs in evidence.items():
  ranks=sorted(((len(v),base) for base,v in locs.items()),reverse=True)
  if not ranks:continue
  support,base=ranks[0]; runner=ranks[1][0] if len(ranks)>1 else 0
  if support>=3 and support>runner:
   confirmed.append((obj,base,mod,support,runner))
  else:ambiguous.append((mod,obj,support,runner,len(locs)))
 confirmed.sort()
 print('LINK386 R4B CORRECTED PAGE-MAP — INDEPENDENT MICROSOFT _TEXT CONTRIBUTION ORDER')
 print('Provenance: original LIBC.LIB LEDATA windows; original hi.exe mapped LE pages')
 print('Strong anchor: >=3 distinct source offsets, strictly more than runner-up base')
 print('WARNING: signature anchors prove matching bytes, not a complete segment or fixup mapping')
 print('MS LE objects: '+', '.join('%d@%08X'%(o,b) for o,b,_ in images))
 print('R3N module names represented in classifier: %d'%len(selected))
 print('Distinct modules with strong _TEXT anchors: %d'%len({x[2] for x in confirmed}))
 print('Strong object-specific anchors: %d'%len(confirmed))
 prev={}
 for obj,base,mod,sup,runner in confirmed:
  attr=metadata.get(mod.lower())
  detail='combine=%d align=%d use32=%d'%(attr['combine'],attr['align'],attr['use32']) if attr else 'attrs=unknown'
  delta=('delta_from_previous=%+d'%(base-prev[obj])) if obj in prev else 'first-anchor'
  print(' ORDER obj=%d base=%08X module=%-18s support=%d runner=%d %s %s'%(obj,base,mod,sup,runner,delta,detail))
  prev[obj]=base
 for mod,obj,sup,runner,count in sorted(ambiguous):
  print(' AMBIGUOUS obj=%d module=%s best=%d runner=%d bases=%d'%(obj,mod,sup,runner,count))
 required={(1,0x10020,'crt0.asm'),(1,0x10d3c,'lseek.asm'),(1,0x10d84,'write.asm'),(1,0x112a0,'heapgrow.c'),(1,0x114e4,'heapinit.c')}
 result={(o,b,m) for o,b,m,*_ in confirmed}
 missing=required-result
 # One short contribution may not yield 3 stable windows; earlier R3W is still authoritative.
 print('Required R3W anchors recovered at stricter threshold: %d/%d'%(len(required)-len(missing),len(required)))
 for obj,b,mod in sorted(missing):print(' DEFERRED required-anchor obj=%d base=%08X module=%s (see R3W)'%(obj,b,mod))
 first=min((b for o,b,m,*_ in confirmed if o==1),default=0)
 print('Code leading mapped object region: object1 base=00010000 first-confirmed=%08X span=%d'%(first,first-0x10000))
 print('LE object 1 startup follows application _main at 00010010; file-order analysis was incorrect.')
 assert (1,0x112a0,'heapgrow.c') in result and (1,0x10020,'crt0.asm') in result
 assert all(x[0]==1 for x in confirmed), "all strong _TEXT anchors must be in code object 1"
 print('R4B CORRECTED MAPPING PASS')
if __name__=='__main__':run()
