#!/usr/bin/env python3
"""R3U: independently inspect actual-vs-predicted import relocation placement.
Read-only. A relocation source site is not an import veneer address.
"""
import collections, pathlib, re, sys
from r3t_correlate import analyze
ROOT=pathlib.Path(__file__).resolve().parents[1]
PAT=re.compile(r'^R3N IMPORT module=(\S+) site=([0-9A-Fa-f]+) loc=([0-9]+) symbol=(\S+)')
def audit(exe, report):
    d,ref,candidates,*_=analyze(exe,report)
    objects=d['objects']
    def object_at(va):
        for o in objects:
            if o['base']<=va<o['base']+o['pages']*d['page_size']:
                return o['index']
        return None
    actual=[]
    for va,fixes in sorted(ref.items()):
        for loc,mode,name,target in fixes:
            if mode != 0: actual.append((va,object_at(va),name,target))
    predicted=[]
    for line in pathlib.Path(report).read_text().splitlines():
        m=PAT.match(line)
        if m:
            va=int(m.group(2),16)
            predicted.append((va,object_at(va),m.group(1),int(m.group(3)),m.group(4)))
    assert len(actual)==len(predicted)==12
    assert len(set(a[0] for a in actual))==12
    ao=collections.Counter(a[1] for a in actual)
    po=collections.Counter(p[1] for p in predicted)
    print('LINK386 R3U IMPORT PLACEMENT AUDIT -- READ ONLY')
    print('ACTUAL LE object split:',dict(sorted(ao.items())))
    print('PREDICTED candidate object split:',dict(sorted(po.items())))
    print('Exact coordinate overlap:',len({a[0] for a in actual}&{p[0] for p in predicted}))
    print('REFERENCE IMPORT SOURCES (absolute VA, object, DLL, ordinal)')
    for va,obj,mod,ordn in actual:
        print(' ACTUAL %08X object=%d %s.%d'%(va,obj,mod,ordn))
    print('PLANNED IMPORT SOURCES (absolute VA, object, OMF module, symbol)')
    for va,obj,mod,loc,sym in predicted:
        print(' PLAN   %08X object=%s module=%s loc=%d symbol=%s'%(va,obj,mod,loc,sym))
    print('MODULE COUNTS')
    for mod,n in sorted(collections.Counter(p[2] for p in predicted).items()):
        print(' MODULE %s=%d'%(mod,n))
    if po != ao:
        print('BLOCKER: candidate object ownership differs from Microsoft output; a uniform delta cannot repair this')
    print('NO relocation patching or linker source modification in R3U')
    return ao,po
if __name__=='__main__':
    exe=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'fixtures/hi-reference.exe'
    report=pathlib.Path(sys.argv[2]) if len(sys.argv)>2 else ROOT/'r3n-hi-report.txt'
    audit(exe,report)
