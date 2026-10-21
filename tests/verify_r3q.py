#!/usr/bin/env python3
"""R3Q verify exact original record and thread interpretation, without fake resolutions."""
import contextlib,io
from pathlib import Path
from r3q_thread_audit import timeline,decode
base=Path(__file__).resolve().parents[1]
s=io.StringIO()
with contextlib.redirect_stdout(s):timeline(base/'fixtures/LIBC.LIB',{'crt0.asm','chkstk.asm','heapgrow.c'})
t=s.getvalue()
assert "('THREAD', 'T', 0, 0, 3, 0)" in t
assert "('FIXUP', 378, 9, 1, 1, 1, 2, 13, None, 44)" in t
assert 'EXTERNAL_INDEX_OUT_OF_RANGE 13 > 12' in t
assert 'DATA 0x2ba type=A3 seg 2 off 0xc encoded 17' in t
assert "('FIXUP', 13, 9, 1, 1, 2, 2, 19, None, 0)" in t
assert 'EXT 0xca total 12' in t
(base/'r3q-thread-audit.txt').write_text(t)
print('R3Q independent FIXUPP THREAD decoder and LIDATA record coordinates: PASS')
