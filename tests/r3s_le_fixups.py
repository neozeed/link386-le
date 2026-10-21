#!/usr/bin/env python3
"""Independent, read-only OS/2 LE fixup/page/object decoder for LINK386 R3S.
Supports observed internal/import-ordinal formats; unknown forms fail closed.
"""
import sys, struct, collections, pathlib

def u16(b,p):return struct.unpack_from('<H',b,p)[0]
def u32(b,p):return struct.unpack_from('<I',b,p)[0]

def decode(path):
 b=pathlib.Path(path).read_bytes();h=u32(b,0x3c)
 assert b[h:h+2]==b'LE'
 np=u32(b,h+0x14);ps=u32(b,h+0x28);objtab=u32(b,h+0x40);no=u32(b,h+0x44)
 pagetab=u32(b,h+0x68);fixrec=u32(b,h+0x6c);modtab=u32(b,h+0x70);nmod=u32(b,h+0x74)
 objects=[]
 for i in range(no):
  p=h+objtab+24*i
  objects.append(dict(index=i+1,size=u32(b,p),base=u32(b,p+4),flags=u32(b,p+8),first=u32(b,p+12),pages=u32(b,p+16)))
 mods=[];p=h+modtab
 for _ in range(nmod):
  n=b[p];p+=1;mods.append(b[p:p+n].decode('ascii'));p+=n
 offsets=[u32(b,h+pagetab+4*i) for i in range(np+1)]
 assert offsets==sorted(offsets) and h+fixrec+offsets[-1]<=len(b)
 pages=[];all_sites=[]
 for pg in range(np):
  p=h+fixrec+offsets[pg];end=h+fixrec+offsets[pg+1];records=[]
  while p<end:
   start=p;src=b[p];flags=b[p+1];p+=2
   listed=bool(src&0x20)
   if listed:
    count=b[p];p+=1
   else:
    sourceoffs=[struct.unpack_from('<h',b,p)[0]];p+=2
   # Target type is in bottom two bits of target flags.
   mode=flags&3
   if mode not in (0,1,2):raise ValueError('unsupported target mode=%d @%x'%(mode,start))
   ordinal_width=1 if flags&0x80 else 2
   module_width=2 if flags&0x40 else 1  # unsupported variation, validate below
   if flags&0x40:raise ValueError('unverified flag bit 0x40 @%x'%start)
   target_index=b[p];p+=1
   if mode==0:
    # LE selector source type 2 has NO target offset.  A reader which
    # consumes one loses record synchronization immediately afterward.
    if (src&0xf)==2:
     target=None
    else:
     width=4 if flags&0x10 else 2
     target=u32(b,p) if width==4 else u16(b,p);p+=width
    if not 1<=target_index<=no:raise ValueError('bad object reference')
    target_name='OBJECT%d'%target_index
   elif mode==1:
    width=4 if flags&0x10 else ordinal_width
    target=u32(b,p) if width==4 else (b[p] if width==1 else u16(b,p));p+=width
    if not 1<=target_index<=nmod:raise ValueError('bad module index')
    target_name=mods[target_index-1]
   else:
    width=4 if flags&0x10 else 2
    target=u32(b,p) if width==4 else u16(b,p);p+=width
    if not 1<=target_index<=nmod:raise ValueError('bad module index')
    target_name=mods[target_index-1]
   if flags&0x04:
    p+=4 if flags&0x20 else 2 # fail instead of silently confusing source-list bit
    raise ValueError('additive records require additional validation @%x'%start)
   if listed:
    sourceoffs=[struct.unpack_from('<h',b,p+2*j)[0] for j in range(count)];p+=count*2
   if p>end:raise ValueError('record crosses fixup page boundary @%x'%start)
   entry=(src,flags,target_name,target,sourceoffs,start)
   records.append(entry)
   for off in sourceoffs:all_sites.append((pg+1,off,src&0xf,mode,target_name,target))
  assert p==end
  pages.append(records)
 return dict(path=str(path),objects=objects,modules=mods,pages=pages,sites=all_sites,page_size=ps,offsets=offsets,header=h)

def main():
 d=decode(sys.argv[1]);print('LE_ORACLE',d['path'],'header=%#x'%d['header'],'page_size=%d'%d['page_size'])
 for o in d['objects']:print('OBJECT',o)
 print('IMPORT_MODULES',d['modules'])
 for i,records in enumerate(d['pages'],1):
  print('PAGE',i,'records',len(records),'sites',sum(len(r[4]) for r in records),'fixup_bytes',d['offsets'][i]-d['offsets'][i-1])
  for src,flags,name,target,offs,fp in records:
   print(' FIXUP','record_offset=%#x'%fp,'source_type=%#x'%(src&0xf),'source_list=%d'%bool(src&0x20),'target_flags=%#x'%flags,'target=%s:%#x'%(name,target),'offsets='+','.join('%+d'%x for x in offs))
 print('SUMMARY records=%d sites=%d internal=%d imported=%d'%(
 sum(map(len,d['pages'])),len(d['sites']),sum(x[3]==0 for x in d['sites']),sum(x[3]!=0 for x in d['sites'])))
if __name__=='__main__':main()
