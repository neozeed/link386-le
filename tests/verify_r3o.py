import pathlib,subprocess,re
r=pathlib.Path(__file__).resolve().parents[1]
def run(xs):
 return subprocess.check_output([str(r/'link386r3o')]+[str(r/x) if not x.startswith('--') else x for x in xs],text=True)
h=run(['fixtures/hi.obj','fixtures/LIBC.LIB','fixtures/OS2386.LIB'])
assert 'module=crt0.asm fixup=1 reason=SITE_OUTSIDE_DATA' in h
assert 'module=chkstk.asm fixup=1 reason=TARGET_EXTERNAL_UNRESOLVED' in h
assert 'module=heapgrow.c fixup=12 reason=TARGET_EXTERNAL_INDEX_INVALID' in h
assert 'R3O SUMMARY total=301 candidate_offset32=286 imports=12 deferred=3 blocked=0 writebacks=0' in h
c=run(['--stack=61440']+[f'fixtures/cmd32/{x}.obj' for x in ('cmd32os2','cmdparse','cmdbatch','cmdfile','cmdos2_os2','cmdos2_session_os2','cmdos2_env')]+['fixtures/LIBC.LIB','fixtures/OS2386.LIB'])
assert re.search(r'R3O SUMMARY total=\d+ candidate_offset32=\d+ imports=\d+ deferred=\d+ blocked=\d+ writebacks=0',c)
print('R3O three-blocker provenance, no-writeback, CMD32: PASS')
