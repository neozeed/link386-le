import subprocess, pathlib, tempfile
root=pathlib.Path(__file__).resolve().parent.parent
exe=root/'link386r2'
def run(*a):
 return subprocess.run([str(exe),*map(str,a)],capture_output=True,text=True)
a=run(root/'fixtures/hi.obj')
assert a.returncode==0,a.stderr
for x in ('MODULE hi.c','SEG 1 _TEXT','GROUP 1 FLAT','PUB _main','EXT _printf UNRESOLVED','FIXUP seg=1 at=00000006','FIXUP seg=1 at=00000001','fixups=2 unresolved=3'):
 assert x in a.stdout,x
b=run(root/'fixtures/import_probe.obj',root/'fixtures/OS2386.LIB')
assert b.returncode==0,b.stderr
assert 'IMPORT Dos32SetMaxFH from DOSCALLS ordinal=209' in b.stdout
assert 'selected_modules=2' in b.stdout and 'unresolved=0' in b.stdout
c=run(root/'fixtures/hi.obj',root/'fixtures/OS2286.LIB',root/'fixtures/OS2386.LIB')
assert c.returncode==0,c.stderr
assert 'selected_modules=1' in c.stdout
with tempfile.TemporaryDirectory() as td:
 p=pathlib.Path(td)/'bad.obj';data=bytearray((root/'fixtures/hi.obj').read_bytes());data[5]^=1;p.write_bytes(data)
 assert run(p).returncode!=0
 p.write_bytes(data[:15]);assert run(p).returncode!=0
print('R2 PASS: real OMF32 data/fixups, both historical libraries, lazy import selection, corruption/truncation rejection')
