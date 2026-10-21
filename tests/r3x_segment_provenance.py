#!/usr/bin/env python3
"""Read-only Microsoft OMF SEGDEF/GRPDEF and LE provenance: R3X.
A GRPDEF with no members is NOT proof of an LE object boundary.
"""
from pathlib import Path
from r3p_raw_audit import members
from r3v_contribution_fingerprints import scan
from r3s_le_fixups import decode
ROOT=Path(__file__).resolve().parents[1]

def index(b,p):
    if p>=len(b): raise ValueError('truncated index')
    x=b[p];p+=1
    if x&128:
        if p>=len(b):raise ValueError('truncated index')
        return ((x&127)<<8)|b[p],p+1
    return x,p

def parse_module(rs):
    names=[''];segs=[None];groups=[];details=[]
    for off,t,b,end in rs:
        if t==0x96:
            p=0
            while p<len(b):
                n=b[p];p+=1
                if p+n>len(b):raise ValueError('LNAMES truncated')
                names.append(b[p:p+n].decode('latin-1'));p+=n
        elif t in (0x98,0x99):
            p=1;attr=b[0];align=attr>>5;combine=(attr>>2)&7;use32=bool(attr&1)
            if align==0:p+=3
            nbytes=4 if t==0x99 else 2
            size=int.from_bytes(b[p:p+nbytes],'little');p+=nbytes
            ni,p=index(b,p);ci,p=index(b,p);oi,p=index(b,p)
            segs.append({'name':names[ni], 'class':names[ci], 'overlay':names[oi], 'size':size, 'align':align,'combine':combine,'use32':use32,'record':t})
        elif t in (0x9a,0x9b):
            p=0;gn,p=index(b,p);mem=[]
            while p<len(b):
                c=b[p];p+=1
                if c==0xff:
                    si,p=index(b,p);mem.append(si)
                elif c==0xfe:
                    _,p=index(b,p)
                else:
                    raise ValueError('unknown GRPDEF component %02X'%c)
            groups.append((names[gn],mem))
    return segs,groups

def main():
    seen={}
    for name,start,rs in members(ROOT/'fixtures/LIBC.LIB'):
        if name.lower() in ('crt0.asm','heapgrow.c','heapinit.c','lseek.asm','write.asm','crt0dat.asm','nmsghdr.asm'):
            seen[name.lower()]=parse_module(rs)
    assert len(seen)==7,seen.keys()
    print('LINK386 R3X OMF SEGMENT/GROUP PROVENANCE -- ORIGINAL LIBC.LIB')
    for module in ('crt0.asm','crt0dat.asm','nmsghdr.asm','lseek.asm','write.asm','heapgrow.c','heapinit.c'):
        segs,groups=seen[module]
        print('MODULE',module)
        for i,s in enumerate(segs[1:],1):
            owners=[gn for gn,mem in groups if i in mem]
            print(' SEGDEF %d name=%s class=%s len=%08X use32=%d combine=%d align=%d group=%s'%(i,s['name'],s['class'],s['size'],s['use32'],s['combine'],s['align'],','.join(owners) if owners else '(none)'))
        for gn,mem in groups:print(' GRPDEF %s member_indexes=%s'%(gn,','.join(map(str,mem)) if mem else '(empty)'))
    for m in ('heapgrow.c','heapinit.c'):
        segs,groups=seen[m];s=segs[1]
        assert s['name']=='_TEXT' and s['class']=='CODE' and s['use32'] and s['record']==0x99
        assert all(1 not in mem for _,mem in groups)
    # Preserve earlier fingerprint provenance; enforce object-2 ownership independent of bitness.
    anchors=scan()
    assert 0x202a0 in anchors['heapgrow.c'][('_TEXT',2)]
    assert 0x204e4 in anchors['heapinit.c'][('_TEXT',2)]
    le=decode(ROOT/'fixtures/hi-reference.exe')
    a,b=le['objects'];assert a['flags']==0x2005 and b['flags']==0x2003
    print('MICROSOFT LE OBJECT FLAGS object1=%04X object2=%04X'%(a['flags'],b['flags']))
    print('CHECK: heap modules use 32-bit _TEXT/CODE but no GRPDEF membership for _TEXT')
    print('CHECK: their source-byte fingerprints occur in Microsoft LE object 2')
    print('UNPROVEN: absent GRPDEF membership alone explains the LE object boundary')
    print('UNPROVEN: source OMF FIXUPP32 coordinates of the four data-object imports')
    print('No layout change or relocations written. PASS')
if __name__=='__main__':main()
