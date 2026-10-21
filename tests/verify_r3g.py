#!/usr/bin/env python3
"""R3G deterministic segment coalescing regression, real historical inputs."""
import subprocess
from pathlib import Path
root=Path(__file__).resolve().parent.parent
exe=root/'link386r3g'
def run(objects):
    return subprocess.check_output([str(exe),*map(str,objects),str(root/'fixtures/LIBC.LIB'),str(root/'fixtures/OS2386.LIB')], text=True)
hello=run([root/'fixtures/hi.obj'])
sarien=run(sorted((root/'fixtures/sarien').glob('*.obj')))
for text,contributions in ((hello,120),(sarien,414)):
    summ=[s for s in text.splitlines() if s.startswith('R3G SUMMARY')]
    assert len(summ)==1,summ
    assert f'contributions={contributions}' in summ[0],summ
    assert 'combined_segments=27' in summ[0],summ
    assert text.count('  CONTRIBUTION ')==contributions
    assert 'BOUNDARY-CANDIDATE _edata=' in text
    assert '  PUB ' in text
assert 'commons=2 ' in hello
assert 'commons=34 ' in sarien
print('R3G PASS: hello and 42 Sarien objects, contribution coalescing, public-address inventory')
