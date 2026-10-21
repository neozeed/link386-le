import pathlib, subprocess, re
root=pathlib.Path(__file__).resolve().parents[1]
def run(args):
    return subprocess.check_output([str(root/'link386r3n')]+[str(root/x) if not x.startswith('--') else x for x in args],text=True)
hi=run(['fixtures/hi.obj','fixtures/LIBC.LIB','fixtures/OS2386.LIB'])
s=re.search(r'R3N SUMMARY total=(\d+) candidate_offset32=(\d+) imports=(\d+) deferred=(\d+) blocked=(\d+) writebacks=0',hi)
assert s and int(s[1])==301 and int(s[2])>200 and int(s[3])>0
assert 'R3N CANDIDATE module=' in hi and 'R3N IMPORT module=' in hi
cmd=run(['--stack=61440']+[f'fixtures/cmd32/{x}.obj' for x in ('cmd32os2','cmdparse','cmdbatch','cmdfile','cmdos2_os2','cmdos2_session_os2','cmdos2_env')]+['fixtures/LIBC.LIB','fixtures/OS2386.LIB'])
assert 'R3N SUMMARY total=' in cmd and 'writebacks=0' in cmd
print('R3N real LIBC/OS2386 hello and CMD32 classification: PASS')
