#!/usr/bin/env python3
"""R3H: independent Microsoft LE/MAP oracle audit; no executable generation."""
import argparse, hashlib, json, re, struct
from pathlib import Path

def u16(b,p):return struct.unpack_from('<H',b,p)[0]
def u32(b,p):return struct.unpack_from('<I',b,p)[0]
def inspect_le(path):
 b=Path(path).read_bytes(); assert len(b)>=0x100 and b[:2]==b'MZ',path
 h=u32(b,0x3c);assert b[h:h+2]==b'LE',(path,hex(h))
 # LE header offsets follow the OS/2 LE documented layout.
 n=u32(b,h+0x44); ot=u32(b,h+0x40); assert 0<n<32,(path,n)
 objs=[]
 for i in range(n):
  o=h+ot+i*24
  assert o+24<=len(b),(path,i)
  size,base,flags,page_index,page_count,reserved=struct.unpack_from('<IIIIII',b,o)
  objs.append(dict(number=i+1,size=size,base=base,flags=flags,page_index=page_index,page_count=page_count))
 return dict(file=Path(path).name,length=len(b),md5=hashlib.md5(b).hexdigest(),le_offset=h,
             entry_object=u32(b,h+0x18),entry_offset=u32(b,h+0x1c),
             stack_object=u32(b,h+0x20),stack_offset=u32(b,h+0x24),
             page_size=u32(b,h+0x28),objects=objs)

def inspect_map(path):
 t=Path(path).read_text(encoding='latin1')
 entries=[]
 for ln in t.splitlines():
  m=re.match(r'^\s*([0-9A-F]{4}):([0-9A-F]{8})\s+([0-9A-F]+)H\s+(\S+)\s+(\S+)\s+(16|32)-bit',ln,re.I)
  if m:
   ob,offset,size,name,cls,bits=m.groups()
   entries.append(dict(object=int(ob,16),offset=int(offset,16),size=int(size,16),name=name,cls=cls,bits=int(bits)))
 entry=re.search(r'Program entry point at\s+([0-9A-F]+):([0-9A-F]+)',t,re.I)
 return dict(file=Path(path).name,segments=entries,entry=dict(object=int(entry.group(1),16),offset=int(entry.group(2),16)) if entry else None)

def compare(m,e):
 problems=[]
 for ob in sorted(set(s['object'] for s in m['segments'])):
  ss=[s for s in m['segments'] if s['object']==ob]
  if ob>len(e['objects']):problems.append('MAP object %d missing from LE'%ob);continue
  maximum=max(s['offset']+s['size'] for s in ss)
  if maximum>e['objects'][ob-1]['size']:
   problems.append('MAP object %d extent 0x%X exceeds LE virtual size 0x%X'%(ob,maximum,e['objects'][ob-1]['size']))
 if m['entry'] and (m['entry']['object']!=e['entry_object'] or m['entry']['offset']!=e['entry_offset']):
  problems.append('MAP entry does not match LE entry')
 return problems

def main():
 a=argparse.ArgumentParser();a.add_argument('--map',action='append',default=[]);a.add_argument('--exe',action='append',default=[]);a.add_argument('--output');args=a.parse_args()
 exes=[inspect_le(p) for p in args.exe]; maps=[inspect_map(p) for p in args.map]
 for x in exes:
  print('LE %s bytes=%d entry=%d:%08X stack=%d:%08X objects=%d'%(x['file'],x['length'],x['entry_object'],x['entry_offset'],x['stack_object'],x['stack_offset'],len(x['objects'])))
  for o in x['objects']: print('  OBJECT %(number)d base=%(base)08X size=%(size)08X flags=%(flags)08X pages=%(page_count)d'%o)
 for m in maps:
  print('MAP %s segments=%d'%(m['file'],len(m['segments'])))
  for seg in m['segments']:print('  %d:%08X +%08X %-24s %s %d'%(seg['object'],seg['offset'],seg['size'],seg['name'],seg['cls'],seg['bits']))
  matching=[e for e in exes if e['file'].lower()==m['file'].lower().replace('.map','.exe')]
  if matching:
   err=compare(m,matching[0]);print('MAP/LE CROSSCHECK:', 'PASS' if not err else 'FAIL '+repr(err))
   if err:raise SystemExit(1)
 if args.output:Path(args.output).write_text(json.dumps(dict(executables=exes,maps=maps),indent=2)+'\n')
if __name__=='__main__':main()
