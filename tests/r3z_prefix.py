#!/usr/bin/env python3
"""R3Z read-only evidence: classify mapped object prefix and cross-object _TEXT.
No source module layout assumption or fixup patching.
"""
from pathlib import Path
from r3s_le_fixups import decode,u32
from r3y_order import run as r3y_run
ROOT=Path(__file__).resolve().parents[1]
def main():
 f=ROOT/'fixtures/hi-reference.exe'; b=f.read_bytes(); d=decode(f)
 page_data=u32(b,d['header']+0x80); ps=d['page_size']; obs=d['objects']
 assert len(obs)==2 and obs[0]['base']==0x10000 and obs[1]['base']==0x20000
 assert ps==4096 and obs[0]['pages']==2
 first=b[page_data:page_data+ps]; second=b[page_data+ps:page_data+2*ps]
 assert len(first)==len(second)==ps
 text=b'MS Run-Time Library'; pos=first.find(text)
 assert pos>=0
 # C-runtime banner and app literal are in the page BEFORE crt0's main fingerprint.
 hello=first.find(b'hi!\n'); assert hello>=0
 # Record what is actually present in each region; don't call page 1 padding.
 nz=sum(v!=0 for v in first)
 first_nonzero=next(i for i,v in enumerate(first) if v)
 code_at=second.find(bytes.fromhex('68 44 00 02 00 e8 4e 05 00 00 83 c4 04 c3'))
 assert code_at>=0
 print('LINK386 R3Z -- INDEPENDENT LE PAGE/PREFIX EVIDENCE')
 print('reference=%s size=%d'%(f.name,len(b)))
 print('object1 base=%08X logical=%d pages=%d'%(obs[0]['base'],obs[0]['size'],obs[0]['pages']))
 print('object2 base=%08X logical=%d pages=%d'%(obs[1]['base'],obs[1]['size'],obs[1]['pages']))
 print('object1 page1: mapped=%08X..%08X nonzero=%d first_nonzero=+%04X'%(0x10000,0x11000,nz,first_nonzero))
 print('runtime banner @%08X'%(0x10000+pos))
 print('hello literal @%08X'%(0x10000+hello))
 print('object1 page2: mapped=%08X..%08X observed startup-byte-sequence @%08X'%(0x11000,0x12000,0x11000+code_at))
 print('R3Y strongly anchored crt0.asm base=00011020, code-import offset delta=00001010 (observed earlier)')
 print('CONCLUSION: object1 initial page carries initialized content, NOT simply padding.')
 print('CAUTION: a byte sequence is not proof of a unique module boundary or segment identity.')
 print('CAUTION: object2 code bytes do not establish a generic OMF-to-LE placement algorithm.')
 print('NO relocation writebacks or executable output attempted.')
 print('R3Z PASS')
if __name__=='__main__':main()
