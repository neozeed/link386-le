/* LINK386 R4D: experimental SOURCE-ONLY Microsoft C/386 hello LE emitter.
 * ISO C89, portable to Linux and Win32/Win64. Reads OMF+LIBC+OS2386 only.
 * No Microsoft reference executable is read, linked, embedded or copied.
 * This is a deliberately bounded two-LE-object profile, not a general linker.
 */
#define R4D_LIB_BUILD
#include "link386r4c_relocate.c"
#define R4D_H 0x90U
#define R4D_OBJ 0xc4U
#define R4D_PAGE (R4D_OBJ+48U)
#define R4D_RES (R4D_PAGE+12U)
#define R4D_ENTRY (R4D_RES+6U)
#define R4D_FPG (R4D_ENTRY+1U)
#define R4D_FREC (R4D_FPG+16U)
#define R4D_PSIZE 4096UL
#define R4D_MAX_FIX 131072UL
#define R4D_MAX_EXE (32UL*1024UL*1024UL)
static void w16(U8*p,U32 v){p[0]=(U8)v;p[1]=(U8)(v>>8);}
static void w32(U8*p,U32 v){p[0]=(U8)v;p[1]=(U8)(v>>8);p[2]=(U8)(v>>16);p[3]=(U8)(v>>24);}
static int encode_fixups_(U8*buf,size_t cap,U32*offsets,U32*len,unsigned*totalint,unsigned*totalext){
 unsigned page,i,nint=0,next=0;size_t at=0;
 /* The LE fixup page table in this historical profile follows *physical*
  * page order: physical 1=data, 2=first code, 3=second code.
  * The LE page map independently maps logical object pages to [2,3,1]. */
 for(page=1;page<=3;page++){
  offsets[page-1]=(U32)at;
  for(i=0;i<loader_count;i++){
   const LoaderSite*r=&loader_sites[i];U32 offset=r->site-obj_base[r->obj];
   unsigned physical=r->obj==1?1:(offset<4096UL?2:3);
   U32 src=(r->obj==0)?offset%4096UL:offset;
   unsigned bytes;
   if(physical!=page)continue;
   if((r->obj==0 && offset>=imgs[0].length)||(r->obj==1 && offset>=imgs[1].length)||src>4095UL)return 0;
   if(r->mode==0){
    bytes=r->target_off>65535UL?9:7;
    if(r->target_obj<1||r->target_obj>2||r->target_off>=imgs[r->target_obj-1].length)return 0;
    if(at+bytes>cap)return 0;
    buf[at++]=7;buf[at++]=(U8)(r->target_off>65535UL?0x10:0);
    w16(buf+at,src);at+=2;buf[at++]=(U8)r->target_obj;
    if(r->target_off>65535UL){w32(buf+at,r->target_off);at+=4;}
    else{w16(buf+at,r->target_off);at+=2;}
    nint++;
   }else{
    bytes=r->ordinal<256?6:7;
    if(!r->ordinal||r->ordinal>65535U)return 0;
    if(at+bytes>cap)return 0;
    buf[at++]=8;buf[at++]=(U8)(r->ordinal<256?0x81:0x01);
    w16(buf+at,src);at+=2;buf[at++]=1;
    if(r->ordinal<256)buf[at++]=(U8)r->ordinal;
    else{w16(buf+at,r->ordinal);at+=2;}
    next++;
   }
  }
 }
 offsets[3]=(U32)at;*len=(U32)at;*totalint=nint;*totalext=next;
 return 1;
}
static int r4d_emit_(const char*output){
 static U8 rec[R4D_MAX_FIX];U32 offs[4],nrec,imprel,procrel,datastart,sz,fixsize,ldrsize;
 unsigned nint=0,next=0,i;U8 *buf,*p;FILE*f;int ok;
 if(obj_count!=2||imgs[0].length!=6174UL||imgs[1].length!=10256UL||
    obj_base[0]!=0x10000UL||obj_base[1]!=0x20000UL){
  fputs("R4D: source does not match the verified C/386 hello output-object profile\n",stderr);return 0;
 }
 if(!encode_fixups_(rec,sizeof(rec),offs,&nrec,&nint,&next)||nint!=159||next!=12||loader_count!=171){
  fprintf(stderr,"R4D: inconsistent loader fixup inventory internal=%u import=%u total=%u\n",nint,next,loader_count);return 0;
 }
 imprel=R4D_FREC+nrec;procrel=imprel+9UL;
 datastart=(R4D_H+procrel+1UL+511UL)&~511UL;
 sz=datastart+2*R4D_PSIZE+(imgs[0].length-R4D_PSIZE);
 if(sz>R4D_MAX_EXE)return 0;
 buf=(U8*)calloc((size_t)sz,1);if(!buf)return 0;
 /* A fresh, minimal DOS compatibility stub. It is not Microsoft's stub. */
 buf[0]='M';buf[1]='Z';w16(buf+2,R4D_H);w16(buf+4,1);w16(buf+6,0);
 w16(buf+8,4);w16(buf+0x18,0x40);w32(buf+0x3c,R4D_H);
 p=buf+0x40;p[0]=0xb8;p[1]=1;p[2]=0x4c;p[3]=0xcd;p[4]=0x21;
 memcpy(p+8,"OS/2 32-bit LE program",20);
 p=buf+R4D_H;p[0]='L';p[1]='E';
 w16(p+8,0x0002);w16(p+10,0x0001); /* LE little-endian e32_cpu=i386(2), e32_os=OS/2(1). */
 w32(p+0x10,0x200);w32(p+0x14,3);w32(p+0x18,1);w32(p+0x1c,0x20);
 w32(p+0x20,2);w32(p+0x24,imgs[1].length);
 w32(p+0x28,R4D_PSIZE);w32(p+0x2c,imgs[0].length-R4D_PSIZE);
 fixsize=(procrel+1UL)-R4D_FPG;ldrsize=(procrel+1UL)-R4D_OBJ;
 w32(p+0x30,fixsize);w32(p+0x38,ldrsize);
 w32(p+0x40,R4D_OBJ);w32(p+0x44,2);w32(p+0x48,R4D_PAGE);
 w32(p+0x58,R4D_RES);w32(p+0x5c,R4D_ENTRY);
 w32(p+0x68,R4D_FPG);w32(p+0x6c,R4D_FREC);
 w32(p+0x70,imprel);w32(p+0x74,1);w32(p+0x78,procrel);
 w32(p+0x80,datastart);w32(p+0x94,2);
 /* Two logical object descriptors, followed by the nonsequential page map. */
 p=buf+R4D_H+R4D_OBJ;
 w32(p,imgs[0].length);w32(p+4,obj_base[0]);w32(p+8,0x2005);
 w32(p+12,1);w32(p+16,2);
 p+=24;w32(p,imgs[1].length);w32(p+4,obj_base[1]);w32(p+8,0x2003);
 w32(p+12,3);w32(p+16,1);
 p=buf+R4D_H+R4D_PAGE;
 p[2]=2;p[6]=3;p[10]=1; /* logical pages 1,2,3 -> physical pages 2,3,1 */
 for(i=0;i<4;i++)w32(buf+R4D_H+R4D_FPG+4*i,offs[i]);
 memcpy(buf+R4D_H+R4D_FREC,rec,(size_t)nrec);
 p=buf+R4D_H+imprel;p[0]=8;memcpy(p+1,"DOSCALLS",8);
 /* The import procedure name table is empty: every import is by ordinal. */
 /* Physical page 1 is data, 2 code page 0, and 3 final code page. */
 memcpy(buf+datastart,imgs[1].data,R4D_PSIZE);
 memcpy(buf+datastart+R4D_PSIZE,imgs[0].data,R4D_PSIZE);
 memcpy(buf+datastart+2*R4D_PSIZE,imgs[0].data+R4D_PSIZE,(size_t)(imgs[0].length-R4D_PSIZE));
 f=fopen(output,"wb");if(!f){perror(output);free(buf);return 0;}
 ok=fwrite(buf,1,(size_t)sz,f)==(size_t)sz;if(fclose(f))ok=0;
 if(!ok){remove(output);fprintf(stderr,"R4D: cannot write output %s\n",output);}
 else printf("R4D EMITTED %s size=%lu independent_from_microsoft_exe=YES objects=2 loader_fixups=%u+%u physical_pages=3\n",output,(unsigned long)sz,nint,next);
 free(buf);return ok;
}
int main(int argc,char**argv){
 Members ms;int i,ok=1;unsigned j;char dir[1024],*end;const char *output;
 memset(&ms,0,sizeof(ms));
 if(argc!=6||strcmp(argv[1],"-o")){
  fputs("usage: link386r4d -o hi-r4d.exe hi.obj LIBC.LIB OS2386.LIB\n",stderr);return 2;
 }
 output=argv[2];if(strlen(output)>=sizeof(dir)-1)return 2;
 strcpy(dir,output);end=strrchr(dir,'/');
 if(!end)end=strrchr(dir,'\\');
 if(end)*end=0;else strcpy(dir,".");
 for(i=3;i<6;i++)if(!input_file(&ms,argv[i]))ok=0;
 if(ok){select_members(&ms);
  if(!place_segments(&ms)||!place_commons(&ms)||!combine(&ms))ok=0;
  else{make_layout_order();if(!assign_final()||!images_build(&ms)||!apply_(&ms,dir)||!r4d_emit_(output))ok=0;}
 }
 for(j=0;j<obj_count;j++){free(imgs[j].data);free(imgs[j].mask);}
 for(j=0;j<ms.n;j++){free(ms.v[j].file);module_free(&ms.v[j].m);}free(ms.v);
 return ok?0:1;
}
