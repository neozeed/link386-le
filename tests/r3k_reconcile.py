#!/usr/bin/env python3
"""Microsoft MAP versus portable OMF combination plan; diagnostic, never modifies bytes."""
import argparse, pathlib, re, subprocess, sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
def parse_map(p):
    rows=[]
    rx=re.compile(r'^\s*([0-9A-F]{4}):([0-9A-F]{8})\s+([0-9A-F]{8,9})H\s+(\S+)\s+(\S+)\s+(16|32)-bit',re.I)
    for line in pathlib.Path(p).read_text(errors='replace').splitlines():
        m=rx.match(line)
        if m: rows.append(dict(obj=int(m[1],16),off=int(m[2],16),size=int(m[3],16),name=m[4],cls=m[5],bits=int(m[6])))
    return rows
def parse_plan(s):
    out=[]; contrib=[]
    rx=re.compile(r'OUTSEG \d+ name=(\S+) class=(\S+) use32=(\d+) combine=(\d+) align=(\d+) offset=([0-9A-F]+) size=(\d+)')
    rc=re.compile(r'CONTRIBUTION module=(.*?) seg=(\S+) outseg=(\d+) local=([0-9A-F]+) region_offset=([0-9A-F]+) size=(\d+)')
    for line in s.splitlines():
        m=rx.search(line)
        if m:out.append(dict(name=m[1],cls=m[2],bits=32 if m[3]=='1' else 16,combine=int(m[4]),align=int(m[5]),off=int(m[6],16),size=int(m[7])))
        m=rc.search(line)
        if m:contrib.append(dict(module=m[1],seg=m[2],out=int(m[3]),local=int(m[4],16),off=int(m[5],16),size=int(m[6])))
    return out,contrib
def run(mapfile, objs, stack):
    cmd=[str(ROOT/'link386r3g'),f'--stack={stack}']+[str(x) for x in objs]
    p=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
    if p.returncode:raise RuntimeError(p.stderr)
    ms=parse_map(mapfile); os,contrib=parse_plan(p.stdout)
    print('LINK386 R3K MAP RECONCILIATION (diagnostic; no executable output)')
    print('Microsoft map:',mapfile)
    print('Source contributions:',len(contrib),'combined:',len(os),'map segments:',len(ms))
    mismatches=[]
    for x in ms:
        cand=[y for y in os if y['name']==x['name'] and y['cls']==x['cls'] and y['bits']==x['bits']]
        if len(cand)!=1:
            print(f"UNMATCHED {x['name']} class={x['cls']} bits={x['bits']} candidates={len(cand)}")
            continue
        y=cand[0]
        delta=x['size']-y['size']
        status='MATCH' if delta==0 else 'DIFFER'
        print(f"{status} {x['name']} class={x['cls']} bits={x['bits']} planned={y['size']} Microsoft={x['size']} delta={delta:+d} map={x['obj']:04X}:{x['off']:08X}")
        if delta:mismatches.append((x,y,delta))
    for x,y,delta in mismatches:
        if x['name']=='_TEXT':
            members=[z for z in contrib if z['seg']=='_TEXT']
            if members:
                last=max(members,key=lambda z:z['local']+z['size'])
                print(f"TEXT-TAIL last-contribution module={last['module']} local={last['local']:08X} size={last['size']} end={last['local']+last['size']:08X}")
                print(f"TEXT-TAIL planned-end={y['size']:08X} Microsoft-end={x['size']:08X} missing={delta}")
                print('TEXT-TAIL cause=UNPROVEN; not inserting fabricated bytes')
    return mismatches
if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--map',required=True);ap.add_argument('--stack',type=int,default=61440);ap.add_argument('inputs',nargs='+');a=ap.parse_args()
    run(a.map,a.inputs,a.stack)
