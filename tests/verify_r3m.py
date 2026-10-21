import subprocess, pathlib, re
root=pathlib.Path(__file__).resolve().parents[1]
def run(args):
 p=subprocess.run([str(root/'link386r3g')]+[x if x.startswith('--') else str(root/x) for x in args],capture_output=True,text=True,check=True)
 return p.stdout
h=run(['fixtures/hi.obj','fixtures/LIBC.LIB','fixtures/OS2386.LIB'])
assert 'R3M OBJECT index=1 base=00010000' in h
assert 'R3M OBJECT index=2 base=00020000' in h
assert 'R3M SYMBOL name=_main ' in h
assert 'R3M SUMMARY objects=2' in h
c=run(['--stack=61440','fixtures/cmd32/cmd32os2.obj','fixtures/cmd32/cmdparse.obj','fixtures/cmd32/cmdbatch.obj','fixtures/cmd32/cmdfile.obj','fixtures/cmd32/cmdos2_os2.obj','fixtures/cmd32/cmdos2_session_os2.obj','fixtures/cmd32/cmdos2_env.obj','fixtures/LIBC.LIB','fixtures/OS2386.LIB'])
assert 'R3M OBJECT index=2 base=00020000' in c
assert 'R3M OBJECT index=3 base=00030000' in c
assert 'R3M GROUP name=DGROUP' in c
assert re.search(r'R3M SEGMENT .*name=\$C20006_CODE16', c)
assert 'R3M COMMON name=__heap_regions' in c
assert 'R3M SUMMARY objects=3' in c
print('R3M final-address candidate fixture checks: PASS')
