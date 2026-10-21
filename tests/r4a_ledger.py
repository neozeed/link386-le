#!/usr/bin/env python3
"""R4A read-only OMF contribution and fixup record ledger."""
import csv,json,re,argparse
from pathlib import Path
from collections import defaultdict
from r3p_raw_audit import members,records
from r3x_segment_provenance import parse_module,index
from r3s_le_fixups import decode
ROOT=Path(__file__).resolve().parents[1]
def selected():
 names=[]
 for line in (ROOT/'r3n-hi-report.txt').read_text().splitlines():
  m=re.search(r'^R3N (?:CANDIDATE|IMPORT|BLOCK|DEFER) module=(\S+)',line)
  if m and m.group(1) not in names:names.append(m.group(1))
 return names
def run(out):
 lib={n.lower():(n,start,rs,'LIBC.LIB') for n,start,rs in members(ROOT/'fixtures/LIBC.LIB')}
 lib['hi.c']=('hi.c',0,list(records((ROOT/'fixtures/hi.obj').read_bytes(),0,(ROOT/'fixtures/hi.obj').stat().st_size)),'hi.obj')
 segrows=[];events=[];unknown=[]
 for mod in selected():
  if mod.lower() not in lib:unknown.append(mod);continue
  n,base,rs,archive=lib[mod.lower()];segs,groups=parse_module(rs);ownership=defaultdict(list)
  for gn,members_ in groups:
   for si in members_:ownership[si].append(gn)
  for i,s in enumerate(segs[1:],1):
   segrows.append(dict(module=n,archive=archive,archive_offset=base,segment_index=i,segment=s['name'],class_name=s['class'],size=s['size'],use32=int(s['use32']),align=s['align'],combine=s['combine'],groups=';'.join(ownership[i])))
  last=None
  for at,t,p,end in rs:
   if t in (0xa0,0xa1,0xa2,0xa3):
    si,q=index(p,0);width=4 if t in (0xa1,0xa3) else 2
    if q+width>len(p):raise ValueError('short data offset')
    off=int.from_bytes(p[q:q+width],'little');last=(si,off,t)
    events.append(dict(module=n,archive=archive,file_offset=at,record_type='%02X'%t,event='DATA',segment_index=si,segment_offset=off,encoded_payload_size=len(p)-q-width,preceding_data_type='',preceding_data_offset=''))
   elif t in (0x9c,0x9d):
    events.append(dict(module=n,archive=archive,file_offset=at,record_type='%02X'%t,event='FIXUPP',segment_index=last[0] if last else '',segment_offset='',encoded_payload_size=len(p),preceding_data_type='%02X'%last[2] if last else '',preceding_data_offset=last[1] if last else ''))
 out.mkdir(parents=True,exist_ok=True)
 for fname,rows in [('segments.csv',segrows),('omf_events.csv',events)]:
  with (out/fname).open('w',newline='') as f:
   writer=csv.DictWriter(f,fieldnames=rows[0].keys());writer.writeheader();writer.writerows(rows)
 # Carry R3Y fingerprint anchors as evidence, never as final placement decisions.
 anchor_rows=[]
 for line in (ROOT/'r3y-order.txt').read_text().splitlines():
  m=re.match(r' ORDER obj=(\d+) base=([0-9A-F]+) module=(\S+)\s+support=(\d+) runner=(\d+)',line)
  if m:
   ob,base,mod,support,runner=m.groups()
   anchor_rows.append(dict(module=mod,le_object=int(ob),candidate_va=int(base,16),distinct_windows=int(support),runner_up=int(runner),evidence='R3Y fingerprint; not a relocation address'))
 with (out/'fingerprint_anchors.csv').open('w',newline='') as f:
  writer=csv.DictWriter(f,fieldnames=anchor_rows[0].keys());writer.writeheader();writer.writerows(anchor_rows)
 oracle=decode(ROOT/'fixtures/hi-reference.exe')
 summary=dict(fingerprint_anchors=len(anchor_rows),selected_names=len(selected()),segment_rows=len(segrows),data_records=sum(x['event']=='DATA' for x in events),fixupp_records=sum(x['event']=='FIXUPP' for x in events),unmatched_names=unknown,le_fixup_records=sum(map(len,oracle['pages'])),le_sites=len(oracle['sites']),le_import_sites=sum(s[3]!=0 for s in oracle['sites']))
 (out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
 assert summary['le_fixup_records']==78 and summary['le_sites']==171 and summary['le_import_sites']==12
 assert any(x['module']=='heapgrow.c' and x['segment']=='_TEXT' for x in segrows)
 assert any(x['module']=='crt0.asm' and x['event']=='FIXUPP' for x in events)
 print('R4A PASS',json.dumps(summary))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--out',default=str(ROOT/'r4a-ledger'));a=p.parse_args();run(Path(a.out))
