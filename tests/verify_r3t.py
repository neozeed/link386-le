from pathlib import Path
from r3t_correlate import analyze
root=Path(__file__).resolve().parents[1]
d,ref,pred,mods,imports,internal,pi,ps=analyze(root/'fixtures/hi-reference.exe',root/'r3n-hi-report.txt')
assert sum(map(len,d['pages']))==78
assert len(d['sites'])==171 and len(imports)==12 and len(internal)==159
assert len(pred)==298
assert len(set(ref)&(pi|ps))==0
assert len(pi)==12
for name in ('crt0.asm','chkstk.asm','heapgrow.c'):
 assert name in mods
print('R3T coordinate reconciliation PASS (read-only, zero exact overlap reported)')
