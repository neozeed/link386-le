#!/usr/bin/env python3
"""R4C strict regression: validates independently materialized images, source edits,
checks the Microsoft LE only after all source output is constructed."""
from pathlib import Path
import tempfile,subprocess,sys,hashlib
from r4c_compare import image
ROOT=Path(__file__).resolve().parents[1]
def go(bin,dir,obj='hi.obj',libc='LIBC.LIB'):
 return subprocess.run([str(ROOT/bin),str(dir),str(ROOT/'fixtures'/obj),str(ROOT/'fixtures'/libc),str(ROOT/'fixtures/OS2386.LIB')],capture_output=True,text=True)
def cmp(p):
 ref=image(ROOT/'fixtures/hi-reference.exe');expected_bad_data=[0x58,0x59,0x5a,0x3f0,0x3f1,0x3f4,0x3f5,0x3f8,0x3f9,0x3fc,0x401,0x404,0x405];out=[]
 for i in (1,2):
  b=(p/f'object{i}-unrelocated.bin').read_bytes();m=(p/f'object{i}-mask.bin').read_bytes();oracle=ref[i-1][2].ljust(len(b),b'\0')
  assert len(b)==len(m)
  assert len(b)==({1:6174,2:10256}[i])
  nonrel=[at for at in range(len(b)) if m[at]==1 and b[at]!=oracle[at]]
  unclaimed=[at for at in range(len(b)) if m[at]==0 and b[at]!=oracle[at]]
  assert not unclaimed,(i,unclaimed[:20])
  assert nonrel==([] if i==1 else expected_bad_data),(i,nonrel)
  out.append((i,m.count(1),m.count(2),m.count(0),len(nonrel)))
 return out

def run():
 with tempfile.TemporaryDirectory(prefix='link386-r4c-') as temp:
  a=Path(temp)/'a';a.mkdir();b=Path(temp)/'b';b.mkdir();c=Path(temp)/'c';c.mkdir()
  res=go('link386r4c_images',a);assert res.returncode==0,res.stderr
  sums=cmp(a)
  # The candidate relocation experiment is deliberately separate and must not
  # be represented as successfully linked/runnable. Count oracle agreement.
  cand=go('link386r4c_relocate',a)
  assert cand.returncode==0,cand.stderr
  assert 'candidate_applied=284' in cand.stdout,cand.stdout[-600:]
  assert 'import_deferred=12 other_deferred=5' in cand.stdout
  from csv import DictReader
  with (a/'relocation-candidates.tsv').open() as f:
   choices=list(DictReader(f,delimiter='\t'))
  assert len(choices)==301
  refs=image(ROOT/'fixtures/hi-reference.exe')
  successes=failures=0
  for rec in choices:
   if rec['result']!='CANDIDATE':continue
   va=int(rec['site'],16);ob=1 if va<0x20000 else 2;off=va-(ob*0x10000)
   built=(a/f'object{ob}-candidate-relocated.bin').read_bytes()[off:off+4]
   wanted=refs[ob-1][2].ljust(10256,b'\0')[off:off+4]
   if built==wanted:successes+=1
   else:failures+=1
  assert (successes,failures)==(223,61),(successes,failures)
  # Reproducible and independent of Microsoft LE reference input.
  again=go('link386r4c_images',b);assert again.returncode==0,again.stderr
  for i in (1,2):
   assert (a/f'object{i}-unrelocated.bin').read_bytes()==(b/f'object{i}-unrelocated.bin').read_bytes()
   assert (a/f'object{i}-mask.bin').read_bytes()==(b/f'object{i}-mask.bin').read_bytes()
  change=go('link386r4c_images',c,obj='hi-yo.obj');assert change.returncode==0,change.stderr
  normal=(a/'object2-unrelocated.bin').read_bytes();variant=(c/'object2-unrelocated.bin').read_bytes()
  assert normal[0x44:0x49]==b'hi!\n\0';assert variant[0x44:0x49]==b'yo!\n\0'
  diff=[i for i in range(len(normal)) if normal[i]!=variant[i]]
  assert diff==[0x44,0x45],diff
  # Deliberate input corruption must reject (checksum).
  damaged=bytearray((ROOT/'fixtures/hi.obj').read_bytes());damaged[24]^=1;bad=Path(temp)/'bad.obj';bad.write_bytes(damaged)
  d=Path(temp)/'d';d.mkdir()
  fail=subprocess.run([str(ROOT/'link386r4c_images'),str(d),str(bad),str(ROOT/'fixtures/LIBC.LIB'),str(ROOT/'fixtures/OS2386.LIB')],capture_output=True,text=True)
  assert fail.returncode!=0,'corrupt OMF input unexpectedly accepted'
  print('R4C VERIFY PASS; source-only independent images; byte-level oracle checks; source edit and checksum negative')
  print(f'DIAGNOSTIC RELOCATION CANDIDATES: {successes} oracle-matching and {failures} mismatching; NOT RUNNABLE')
  for i,regular,fix,missing,mismatch in sums:print(f'OBJECT{i}: size={[6174,10256][i-1]} direct_bytes={regular} fixup_operand_bytes={fix} unsourced={missing} nonrelocated_mismatches={mismatch}')
if __name__=='__main__':run()
