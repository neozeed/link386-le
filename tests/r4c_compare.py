#!/usr/bin/env python3
"""Compare source-built OMF segment bytes with Microsoft hi.exe virtual LE pages.
Comparison only: Microsoft EXE is not an input to link386r4c or to generated .bin files.
Unresolved external relocations are masked, not patched.
"""
import argparse,csv,collections,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def u32(b,p):return int.from_bytes(b[p:p+4],'little')
def image(exe):
 b=exe.read_bytes();h=u32(b,0x3c);assert b[h:h+2]==b'LE';ps=u32(b,h+0x28);n=u32(b,h+0x14);do=u32(b,h+0x80);pt=h+u32(b,h+0x48);ot=h+u32(b,h+0x40);phys=[int.from_bytes(b[pt+4*i:pt+4*i+3],'big') for i in range(n)]
 result=[]
 for i in range(u32(b,h+0x44)):
  p=ot+24*i;size,base,first,num=u32(b,p),u32(b,p+4),u32(b,p+12),u32(b,p+16)
  buf=b''.join(b[do+(phys[j-1]-1)*ps:do+phys[j-1]*ps].ljust(ps,b'\0') for j in range(first,first+num))
  result.append((i+1,base,buf))
 return result

def readtab(p):
 with p.open(newline='') as f:return list(csv.DictReader(f,delimiter='\t'))
def run(out,exe):
 imgs=image(exe);segs=readtab(out/'contributions.tsv');fx=readtab(out/'fixups.tsv');masks=collections.defaultdict(set)
 for f in fx:
  k=(f['member'],f['seg']);width={9:4,11:4,1:2,2:2,3:2,0:1}.get(int(f['loc']),0);at=int(f['segment_site'])
  if width and f['status']!='BAD_SEGMENT_SITE':masks[k].update(range(at,at+width))
 placed=[];weak=[];missing=[];checked=0
 for s in segs:
  path=s['file'];size=int(s['size']);cls=s['class'];name=s['name'];m=s['module']
  if path=='-' or size<16 or cls not in ('CODE','DATA','CONST','BEGDATA','MSG','FAR_DATA','FAR_BSS','BSS'):continue
  p=Path(path);p=p if p.is_absolute() else ROOT/p
  bb=p.read_bytes();assert len(bb)==size
  mask=masks[(s['member'],s['seg'])]
  # Use unmodified 12-byte windows, >3 distinct byte values, not all 0, as independent evidence.
  witnesses=collections.defaultdict(set)
  for start in range(0,size-11,4):
   if any(x in mask for x in range(start,start+12)):continue
   key=bb[start:start+12]
   if len(set(key))<5:continue
   for obj,base,body in imgs:
    pos=0
    while True:
     at=body.find(key,pos)
     if at<0:break
     cand=at-start
     if cand>=0 and cand+size<=len(body):witnesses[(obj,cand)].add(start)
     pos=at+1
  if not witnesses:missing.append((m,name,size,'NO_WINDOWS'));continue
  ranked=sorted(((len(starts),k) for k,starts in witnesses.items()),reverse=True)
  best,(obj,at)=ranked[0];second=ranked[1][0] if len(ranked)>1 else 0
  if best<2 or best==second:
   weak.append((m,name,size,best,second,'AMBIGUOUS'));continue
  body=imgs[obj-1][2]
  comparable=sum(1 for i in range(size) if i not in mask)
  mismatches=sum(1 for i in range(size) if i not in mask and bb[i]!=body[at+i])
  record=dict(module=m,segment=name,member=int(s['member']),seg=int(s['seg']),object=obj,va=imgs[obj-1][1]+at,size=size,confirmed_windows=best,runner_up_windows=second,verified_unrelocated_bytes=comparable-mismatches,relocated_or_masked_bytes=size-comparable,mismatched_unmasked_bytes=mismatches)
  if mismatches==0 and best>=3:placed.append(record)
  else:weak.append((m,name,size,best,second,f'MISMATCHES={mismatches}'))
  checked+=1
 report=dict(reference=str(exe),assembled_without_reference=True,accepted=sorted(placed,key=lambda z:(z['object'],z['va'])),weak=weak,unmatched=missing,segments_with_candidate=checked,accepted_segments=len(placed),accepted_source_bytes=sum(z['verified_unrelocated_bytes'] for z in placed),masked_fixup_operand_bytes=sum(z['relocated_or_masked_bytes'] for z in placed))
 (out/'reference-comparison.json').write_text(json.dumps(report,indent=2)+'\n')
 print(f'R4C COMPARE accepted_segments={len(placed)} source_bytes_confirmed={report["accepted_source_bytes"]} candidate_segments={checked} unmatched={len(missing)} weak={len(weak)}')
 for z in placed:print(f'ANCHOR {z["module"]:18} {z["segment"]:10} obj={z["object"]} va={z["va"]:08X} source_bytes={z["verified_unrelocated_bytes"]} mask={z["relocated_or_masked_bytes"]} windows={z["confirmed_windows"]}')
 for r in weak:print('DEFER',*r)
 return report
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--out',type=Path,default=ROOT/'build/r4c');ap.add_argument('--reference',type=Path,default=ROOT/'fixtures/hi-reference.exe');a=ap.parse_args();run(a.out,a.reference)
