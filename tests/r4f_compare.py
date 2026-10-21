from pathlib import Path
p=Path('/mnt/data/link386-r4f')
def extract(path):
 b=path.read_bytes();l=int.from_bytes(b[60:64],'little');h=b[l:];u=lambda i:int.from_bytes(h[i:i+4],'little');a=[];pages=[h[u(0x48)+4*i+2] for i in range(u(0x14))]
 for i in range(u(0x44)):
  o=u(0x40)+i*24;n=u(o);pg=u(o+12);cnt=u(o+16);buf=bytearray(n)
  for j in range(min(cnt,len(pages)-pg+1)):
   k=pages[pg+j-1];off=u(0x80)+(k-1)*4096;size=min(4096,n-j*4096)
   if k and off<len(b):buf[j*4096:j*4096+size]=b[off:off+size]
  a.append(buf)
 return a
own=extract(p/'build/sarien-r4f.exe');ref=extract(p/'fixtures/sarien/sarien.exe')
for i,(x,y) in enumerate(zip(own,ref),1):
 mask=(p/'build'/('object%d-r4f-mask.bin'%i)).read_bytes();counts=[0]*3;examples=[[],[],[]]
 for j in range(len(x)):
  if x[j]!=y[j]:
   t=mask[j];counts[t]+=1
   if len(examples[t])<15:examples[t].append(hex(j))
 print('object',i,'unsourced',counts[0],'regular',counts[1],'reloc',counts[2], 'sample',examples)
