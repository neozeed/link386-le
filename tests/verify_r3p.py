import pathlib,subprocess
root=pathlib.Path(__file__).resolve().parents[1]
a=subprocess.check_output(['python3',str(root/'tests/r3p_raw_audit.py'),str(root/'fixtures/LIBC.LIB')],text=True)
assert 'MODULE crt0.asm' in a and 'MODULE chkstk.asm' in a and 'MODULE heapgrow.c' in a
assert 'EXTDEF record=0xAA names=21 total=21' in a
assert 'EXTDEF record=0xCA names=11 total=12' in a
assert '12:__amblksiz' in a
assert 'EXTERNAL TABLE 1:_end 2:__amsg_exit' in a
print('R3P independent raw archive/record provenance: PASS')
