import re,collections
from pathlib import Path
P=Path(__file__).resolve().parents[1]
place=re.compile(r'R4C PLACE module=(\S+) seg=(\S+) group=\S* object=(\d+) offset=([0-9A-F]+) size=(\d+)')
infer=re.compile(r'^OBJECT=(\d+) MODULE=(\S+) SEG=(\S+) SIZE=(\d+) OLD=([0-9A-F]+) BEST=([0-9A-F]+|\?)')
entries=[]
for m,s,o,off,size in place.findall((P/'build/r4g-build.log').read_text()):
 if int(o)!=1:continue
 entries.append(dict(mod=m,seg=s,old=int(off,16),size=int(size),new=None))
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
print('ENTRIES',len(entries),'PLACED',sum(d['new'] is not None for d in entries))
known=sorted((d for d in entries if d['new'] is not None),key=lambda x:x['new'])
for a,b in zip(known,known[1:]):
 gap=b['new']-(a['new']+a['size'])
 if gap!=0:print('GAP',hex(a['new']+a['size']),hex(b['new']),gap,a['mod'],'->',b['mod'])
print('UNKNOWN')
for d in entries:
 if d['new'] is None: print(d)
