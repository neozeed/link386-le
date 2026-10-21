import pathlib, struct, subprocess, tempfile
root=pathlib.Path(__file__).resolve().parents[1]
def rec(t,b):
 z=bytes([t,(len(b)+1)&255,(len(b)+1)>>8])+b
 return z+bytes([(-sum(z))&255])
def name(s):return bytes([len(s)])+s.encode('ascii')
# E8 disp32 followed by RET, intra-segment relative fixup at offset 1.
# LOCAT: FIXUP, self-relative, location=9, offset=1; FIXDAT: frame SEGDEF1, target SEGDEF1, no displacement.
obj=b''.join([rec(0x80,name('rel32')),rec(0x96,name('_TEXT')+name('CODE')+name('FLAT')),
 rec(0x99,b'\xa1'+struct.pack('<I',6)+b'\x01\x02\x03'),
 rec(0x91,b'\x00\x01'+name('_start')+struct.pack('<I',0)+b'\x00'),
 rec(0xa1,b'\x01'+struct.pack('<I',0)+b'\xe8\0\0\0\0\xc3'),
 rec(0x9d,b'\xa4\x01\x04\x01\x01'),rec(0x8b,b'\0')])
with tempfile.TemporaryDirectory() as d:
 p=pathlib.Path(d); (p/'rel32.obj').write_bytes(obj)
 cmd=[str(root/'link386r3'),'-o',str(p/'rel32.exe'),'-e','_start',str(p/'rel32.obj')]
 r=subprocess.run(cmd,capture_output=True,text=True)
 assert r.returncode==0,(r.stdout,r.stderr)
 b=(p/'rel32.exe').read_bytes();offs=struct.unpack_from('<I',b,0x80+0x80)[0]
 assert b[offs:offs+6]==b'\xe8\xfb\xff\xff\xff\xc3',b[offs:offs+6].hex()
 print('R3A intra-object self-relative FIXUPP32 PASS')
