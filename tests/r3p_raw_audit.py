#!/usr/bin/env python3
"""Independent, read-only OMF record timeline for three C/386 runtime blockers."""
from pathlib import Path
import sys

def records(data, start, stop):
    p=start
    while p+3<=stop:
        typ=data[p]; length=int.from_bytes(data[p+1:p+3], 'little'); end=p+3+length
        if not length or end>stop: break
        yield (p,typ,data[p+3:end-1],end)
        p=end
        if typ in (0x8a,0x8b): break

def members(path):
    b=path.read_bytes(); page=int.from_bytes(b[1:3],'little')+3
    if b[0]!=0xf0 or page<16: raise ValueError('not Microsoft OMF library')
    pos=page
    while pos<len(b):
        rs=list(records(b,pos,len(b)))
        if not rs or rs[0][1] not in (0x80,0x82):break
        p=rs[0][2]; name=p[1:1+p[0]].decode('latin-1','replace')
        yield name,pos,rs
        pos=((rs[-1][3]+page-1)//page)*page

def scan(path):
    seen=set()
    for name,start,rs in members(path):
        if name.lower() not in {'crt0.asm','chkstk.asm','heapgrow.c'}:continue
        seen.add(name.lower());print('MODULE',name,'archive_offset=0x%X'%start)
        exts=[]; last_data=None
        for pos,t,p,end in rs:
            if t in (0x8c,0xb4):
                cursor=0; names=[]
                while cursor<len(p):
                    n=p[cursor]; cursor+=1
                    if cursor+n>=len(p):raise ValueError('bad EXTDEF')
                    names.append(p[cursor:cursor+n].decode('latin-1','replace'));cursor+=n
                    # OMF type index
                    if p[cursor]&0x80:cursor+=2
                    else:cursor+=1
                exts.extend(names)
                print('  EXTDEF record=0x%X names=%d total=%d'%(pos-start,len(names),len(exts)))
            elif t in (0xa0,0xa1,0xa2,0xa3):
                last_data=(pos,t,len(p))
                print('  DATA record=0x%X type=%02X encoded_payload=%d'%(pos-start,t,len(p)))
            elif t in (0x9c,0x9d):
                print('  FIXUP record=0x%X type=%02X payload=%s following_data=%s external_table=%d'%(pos-start,t,p.hex(' '), '0x%X/%02X'%(last_data[0]-start,last_data[1]) if last_data else 'NONE',len(exts)))
        print('  EXTERNAL TABLE', ' '.join('%d:%s'%(i+1,s) for i,s in enumerate(exts)))
    assert len(seen)==3,seen
if __name__=='__main__':scan(Path(sys.argv[1] if len(sys.argv)>1 else Path(__file__).resolve().parents[1]/'fixtures/LIBC.LIB'))
