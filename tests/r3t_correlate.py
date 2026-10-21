#!/usr/bin/env python3
"""R3T: compare R3N candidate site coordinates with R3S real LE loader sites.
Read-only; never equates static OMF fixups with loader relocations.
"""
import collections
import pathlib
import re
import sys
from r3s_le_fixups import decode

ROOT = pathlib.Path(__file__).resolve().parents[1]
SITE = re.compile(r'^R3N (CANDIDATE|IMPORT) module=(\S+) site=([0-9a-fA-F]+)\b')

def analyze(exe, report):
    d = decode(exe)
    pages = {}
    for obj in d['objects']:
        for p in range(obj['first'], obj['first'] + obj['pages']):
            pages[p] = (obj['index'], obj['base'], p - obj['first'])
    reference = collections.defaultdict(list)
    for page, offset, loc, mode, name, target in d['sites']:
        obj, base, ordinal = pages[page]
        va = base + ordinal * d['page_size'] + offset
        if not base <= va < base + next(o['pages'] for o in d['objects'] if o['index'] == obj) * d['page_size']:
            raise ValueError('LE fixup outside mapped object pages at %08x' % va)
        reference[va].append((loc, mode, name, target))
    candidates = []
    for line in pathlib.Path(report).read_text().splitlines():
        match = SITE.match(line)
        if match:
            candidates.append((match.group(1), match.group(2), int(match.group(3), 16)))
    if not candidates:
        raise ValueError('no candidate coordinates decoded')
    actual_import = {va for va, fixes in reference.items() if any(f[1] != 0 for f in fixes)}
    actual_internal = set(reference) - actual_import
    pred_import = {va for kind, mod, va in candidates if kind == 'IMPORT'}
    pred_static = {va for kind, mod, va in candidates if kind == 'CANDIDATE'}
    modules = collections.defaultdict(lambda: [0, 0, 0, 0])
    for kind, mod, va in candidates:
        slot = modules[mod]
        slot[0] += 1
        if va in reference:
            slot[1] += 1
        if kind == 'IMPORT':
            slot[2] += 1
            if va in actual_import:
                slot[3] += 1
    return d, reference, candidates, modules, actual_import, actual_internal, pred_import, pred_static

def main():
    exe = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT/'fixtures/hi-reference.exe'
    report = pathlib.Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT/'r3n-hi-report.txt'
    d, ref, pred, modules, imports, internal, pi, ps = analyze(exe, report)
    print('R3T OMF-to-LE site coordinate correlation (READ ONLY)')
    print('LE reference: records=%d sites=%d internal=%d imports=%d' % (sum(map(len,d['pages'])),len(d['sites']),len(internal),len(imports)))
    print('OMF candidates: total=%d static=%d import=%d' % (len(pred),sum(r[0]=='CANDIDATE' for r in pred),sum(r[0]=='IMPORT' for r in pred)))
    print('EXACT loader-site coordinate overlap: any=%d imports=%d' % (len(set(ref)&(ps|pi)),len(imports&pi)))
    print('NOTE: Static OMF relocations can disappear at link time; counts are not directly comparable.')
    print('NOTE: Candidate placement is unverified; an absent overlap is not proof an OMF fixup is invalid.')
    for mod, (n,m,ni,mi) in modules.items():
        print('MODULE %-20s candidate=%3d loader_site_overlap=%3d import=%2d import_overlap=%2d' % (mod,n,m,ni,mi))
    for tag, module in [('crt0.asm','crt0.asm'),('chkstk.asm','chkstk.asm'),('heapgrow.c','heapgrow.c')]:
        rows=[r for r in pred if r[1]==module]
        print('DISPUTED %-12s candidate_sites=%d exact_loader_overlap=%d' % (tag,len(rows),sum(r[2] in ref for r in rows)))
    print('RESULT: comparison complete; do not enable LE writebacks based on candidate coordinates alone')

if __name__=='__main__':main()
