from pathlib import Path
import re
P=Path(__file__).resolve().parents[1]
place=re.compile(r'R4C PLACE module=(\S+) seg=(\S+) group=\S* object=(\d+) offset=([0-9A-F]+) size=(\d+)')
infer=re.compile(r'^OBJECT=(\d+) MODULE=(\S+) SEG=(\S+) SIZE=(\d+) OLD=([0-9A-F]+) BEST=([0-9A-F]+|\?)')
entries=[dict(mod=m,seg=s,old=int(off,16),size=int(size),new=None) for m,s,o,off,size in place.findall((P/'build/r4g-build.log').read_text()) if int(o)==1]
for line in (P/'build/r4k-infer.txt').read_text().splitlines():
 a=infer.match(line)
 if not a:continue
 ob,m,s,size,off,new=a.groups()
 if int(ob)!=1:continue
 for d in entries:
  if (d['mod'],d['seg'],d['size'],d['old'])==(m,s,int(size),int(off,16)):
   d['new']=int(new,16) if new!='?' else None;break
for d in entries:
 if d['new'] is None and d['old']<0xF550:d['new']=d['old']
extra={'crt0msg.asm':0x107CC,'crt0fp.asm':0x107F0,'abort.asm':0x1010C,'hpabort.asm':0x11A90}
for d in entries:
 if d['new'] is None: d['new']=extra[d['mod']]
assert len(entries)==104 and all(d['new'] is not None for d in entries)
entries.sort(key=lambda x:x['new'])
pos=0; mistakes=[]
for d in entries:
 aligned=(pos+3)&~3
 if aligned!=d['new']:mistakes.append((d['mod'],hex(aligned),hex(d['new'])))
 pos=d['new']+d['size']
print('expected mismatches',len(mistakes),mistakes[:20],'last',hex(pos))
inc=P/'src/r4k_order.inc'
with inc.open('w') as f:
 f.write('/* R4K experimental, reference-derived CODE contribution ordering only.\n   This is NOT an independent LINK386 order selection algorithm.\n   Source bytes still come exclusively from supplied OMF inputs. */\n')
 f.write('typedef struct {const char*name;const char*seg;U32 size;U32 refpos;} R4KCodeOrder;\n')
 f.write('static const R4KCodeOrder r4k_code[]={\n')
 for d in entries:f.write(' {"%s","%s",%luUL,0x%lXUL},\n'%(d['mod'],d['seg'],d['size'],d['new']))
 f.write('};\n')
 f.write('static U32 r4k_get_code_key(const char *name,const char*seg,U32 size,unsigned fallback){unsigned k;for(k=0;k<sizeof(r4k_code)/sizeof(r4k_code[0]);k++)if(same_(name,r4k_code[k].name)&&same_(seg,r4k_code[k].seg)&&size==r4k_code[k].size)return r4k_code[k].refpos;return 0xF0000000UL+(U32)fallback;}\n')
