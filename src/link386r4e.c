/* LINK386 R4E: EXPERIMENTAL three-object C/386 source-only LE emitter.
 * Command: link386r4e -o OUT -d PROFILE.DEF [--stack=N] OBJ... LIBC.LIB OS2386.LIB
 * Does not open a prelinked EXE. Supports current CMD32 console-test profile
 * and its DEF ordinal-import bindings. Not yet a general LINK386 workalike. */
#define R4L_LINKER
#define R4D_LIB_BUILD
#include "link386r4e_relocate.c"
#include "link386r4e_def.inc"
#define R4E_PS 4096UL
#define R4E_SIZE_MAX (32UL*1024UL*1024UL)
#define R4E_FIX_MAX (1024UL*1024UL)
static U32 r4e_align512(U32 v){return (v+511UL)&~511UL;}
static void r4e_w16(U8*p,U32 x){p[0]=(U8)x;p[1]=(U8)(x>>8);}
static void r4e_w32(U8*p,U32 x){p[0]=(U8)x;p[1]=(U8)(x>>8);p[2]=(U8)(x>>16);p[3]=(U8)(x>>24);}
/* This profile is based on the actual OMF-selected objects, not the reference
 * executable. Physical page ordering mirrors Microsoft's C/386 linker policy:
 * data first; 32-bit code next; 16-bit bridge last. */
static unsigned r4e_page_count(unsigned ob){U32 initialized=0;unsigned j;
 if(ob==obj_count-1){for(j=0;j<no;j++)if(finalobj[j]==ob&&os[j].kind==1){U32 end=finalseg[j]+os[j].size;if(end>initialized)initialized=end;}
  if(!initialized)initialized=1;
  return (unsigned)((initialized+R4E_PS-1)/R4E_PS);
 }
 return (unsigned)((imgs[ob].length+R4E_PS-1)/R4E_PS);
}
static unsigned r4e_physical(unsigned ob,U32 offset){unsigned i,n=0;
 if(ob==obj_count-1)return (unsigned)(offset/R4E_PS)+1;
 n+=r4e_page_count(obj_count-1);
 for(i=0;i<obj_count-1;i++){
  if(i==ob)return n+(unsigned)(offset/R4E_PS)+1;
  n+=r4e_page_count(i);
 }
 return 0;
}
static int r4e_encode(U8*rec,U32 cap,U32*pageoffs,unsigned pages,U32*reclen,unsigned*ni,unsigned*ne){
 unsigned phys,i,a=0,b=0;U32 pos=0;LoaderSite*r;U32 v,off;unsigned bytes,src,flags,fixwidth,firstphys,secondphys;U32 pageoff;
 for(phys=1;phys<=pages;phys++){
  pageoffs[phys-1]=pos;
  for(i=0;i<loader_count;i++){
   r=&loader_sites[i];if(r->obj>=obj_count){fprintf(stderr,"R4E encode BADOBJ index=%u\n",i);return 0;}
   off=r->site-obj_base[r->obj];if(r->site<obj_base[r->obj]||off>=imgs[r->obj].length){fprintf(stderr,"R4E encode BADSITE index=%u site=%08lX\n",i,(unsigned long)r->site);return 0;}
   /* Internal 16:16 fixups become selector fixups at pointer+2.  That
    * selector word may be on the NEXT page, so remap the effective site
    * before selecting the physical fixup-page table entry. */
   if(!r->mode && r->loc==3){
    if(off+2U>=imgs[r->obj].length)return 0;
    off+=2U;
   }
   /* A fixup spanning a physical page boundary must occur in BOTH page
    * records. On the following page its signed source offset is negative.
    * Microsoft LINK386 does this even when a single OMF fixup is involved. */
   fixwidth=(r->loc==2?2U:(r->loc==11?6U:4U));
   pageoff=off%R4E_PS;
   firstphys=r4e_physical(r->obj,off);
   secondphys=0;
   if(pageoff+fixwidth>R4E_PS){
    if(off+fixwidth>imgs[r->obj].length)return 0;
    if((off/R4E_PS)+1U >= r4e_page_count(r->obj))return 0;
    secondphys=r4e_physical(r->obj,(off/R4E_PS+1U)*R4E_PS);
   }
   if(firstphys==phys)src=(unsigned)pageoff;
   else if(secondphys==phys)src=(unsigned)(pageoff-R4E_PS);
   else continue;
   if(firstphys==0 || (secondphys && firstphys==secondphys))return 0;
   if(r->loc==9)v=r->mode?8:7;
   else if(r->loc==2)v=2;
   else if(r->loc==3){
    /* Microsoft C/386 has already statically relocated the OFFSET half of
     * an internal 16:16 pointer.  Only its selector word remains for LE
     * loader relocation.  Import far pointers still use 16:16 alias source
     * type 0x13 and start at their first word. */
    if(r->mode)v=0x13UL;
    else v=0x12UL;
   }
   else if(r->loc==11)v=6;
   else return 0;
   if(r->mode){
    if(!r->module||r->module>r4e_dll_count||r->module>255||r->ordinal>65535){fprintf(stderr,"R4E encode BADIMPORT index=%u module=%u ordinal=%u\n",i,r->module,r->ordinal);return 0;}
    flags=(r->ordinal<256?0x81:0x01);bytes=r->ordinal<256?6:7;
    if(pos+bytes>cap)return 0;
    rec[pos++]=(U8)v;rec[pos++]=(U8)flags;r4e_w16(rec+pos,src);pos+=2;
    rec[pos++]=(U8)r->module;
    if(r->ordinal<256)rec[pos++]=(U8)r->ordinal;
    else {r4e_w16(rec+pos,r->ordinal);pos+=2;}
    b++;
   }else{
    if(r->target_obj<1||r->target_obj>obj_count||r->target_off>=imgs[r->target_obj-1].length){fprintf(stderr,"R4E encode BADTARGET index=%u src_obj=%u loc=%u site=%08lX obj=%u off=%08lX\n",i,r->obj+1,r->loc,(unsigned long)r->site,r->target_obj,(unsigned long)r->target_off);return 0;}
    /* LE source type 2 (selector) carries ONLY an object index: there is
     * no TRGOFF word.  R4E used to write two extra bytes, making all
     * subsequent records in that page unreadable by a compliant loader. */
    if((v&0x0fU)==2U){flags=0;bytes=5;}
    else {flags=(r->target_off>65535UL?0x10:0);bytes=r->target_off>65535UL?9:7;}
    if(pos+bytes>cap)return 0;
    rec[pos++]=(U8)v;rec[pos++]=(U8)flags;r4e_w16(rec+pos,src);pos+=2;
    rec[pos++]=(U8)r->target_obj;
    if((v&0x0fU)!=2U){
     if(r->target_off>65535UL){r4e_w32(rec+pos,r->target_off);pos+=4;}
     else{r4e_w16(rec+pos,r->target_off);pos+=2;}
    }
    a++;
   }
  }
 }
 pageoffs[pages]=pos;*reclen=pos;*ni=a;*ne=b;return 1;
}
static int r4e_emit(const char*output,const Members*ms){
 static U8 rec[R4E_FIX_MAX];U32 offs[512],recsize,ot=0xc4U,pt,rs,entrytab,pgfix,fxrec,modtab,proctab,datastart,filesize,n;
 U32 entry=0;unsigned entryobj=0,im=0,i,j,opages[3],pages=0,numint=0,numimp=0;
 U8*buf,*p;FILE*f;int ok;
 if(obj_count!=2&&obj_count!=3){fputs("R4F: expected two or three LE objects\n",stderr);return 0;}
 for(i=0;i<obj_count;i++){opages[i]=r4e_page_count(i);pages+=opages[i];}
 if(pages>511||pages<3||r4e_dll_count>32)return 0;
 if(!r4e_encode(rec,sizeof(rec),offs,pages,&recsize,&numint,&numimp)||numint+numimp<loader_count){
  fputs("R4E: LE relocation serialization failed; refusing output\n",stderr);return 0;
 }
 if(!pub_va_(ms,"__astart",&entry,&entryobj,&im)||im||entryobj!=0||entry<obj_base[0]||entry-obj_base[0]>=imgs[0].length){
  fputs("R4E: cannot resolve __astart entry point\n",stderr);return 0;
 }
 pt=ot+24U*obj_count;rs=pt+4U*pages;entrytab=rs+6U;pgfix=entrytab+1U;
 fxrec=pgfix+4U*(pages+1U);modtab=fxrec+recsize;proctab=modtab;
 for(i=0;i<r4e_dll_count;i++)proctab+=(U32)strlen(r4e_dll_names[i])+1;
 datastart=r4e_align512(0x90U+proctab+1U);
 filesize=datastart;
 for(i=0;i<obj_count;i++)filesize+=imgs[i].length; /* upper bound for aligned page storage */
 filesize+=4096UL*pages;
 if(filesize>R4E_SIZE_MAX)return 0;
 buf=(U8*)calloc((size_t)filesize,1);if(!buf)return 0;
 buf[0]='M';buf[1]='Z';r4e_w16(buf+2,0x90);r4e_w16(buf+4,1);r4e_w16(buf+8,4);r4e_w16(buf+0x18,0x40);r4e_w32(buf+0x3c,0x90);
 p=buf+0x40;p[0]=0xb8;p[1]=1;p[2]=0x4c;p[3]=0xcd;p[4]=0x21;memcpy(p+8,"OS/2 32-bit LE program",20);
 p=buf+0x90;p[0]='L';p[1]='E';r4e_w16(p+8,2);r4e_w16(p+10,1);
 r4e_w32(p+0x10,obj_count==2?0x300:0x200);r4e_w32(p+0x14,pages);r4e_w32(p+0x18,entryobj+1);r4e_w32(p+0x1c,entry-obj_base[entryobj]);
 r4e_w32(p+0x20,obj_count);r4e_w32(p+0x24,imgs[obj_count-1].length);
 r4e_w32(p+0x28,R4E_PS);
 /* Last physical page is the code16 bridge (not the data object's BSS). */
 r4e_w32(p+0x2c,imgs[obj_count==2?0:1].length%R4E_PS);
 r4e_w32(p+0x30,proctab+1U-pgfix);r4e_w32(p+0x38,proctab+1U-ot);
 r4e_w32(p+0x40,ot);r4e_w32(p+0x44,obj_count);r4e_w32(p+0x48,pt);
 r4e_w32(p+0x58,rs);r4e_w32(p+0x5c,entrytab);r4e_w32(p+0x68,pgfix);r4e_w32(p+0x6c,fxrec);
 r4e_w32(p+0x70,modtab);r4e_w32(p+0x74,r4e_dll_count);r4e_w32(p+0x78,proctab);
 r4e_w32(p+0x80,datastart);r4e_w32(p+0x94,3);
 for(i=0,n=1;i<obj_count;i++){
  U32 objoff=0x90U+ot+24U*i;
  r4e_w32(buf+objoff,imgs[i].length);r4e_w32(buf+objoff+4,obj_base[i]);
  r4e_w32(buf+objoff+8,i==0?0x2005:(obj_count==3&&i==1?0x1005:0x2003));
  r4e_w32(buf+objoff+12,n);r4e_w32(buf+objoff+16,opages[i]);n+=opages[i];
 }
 /* LE logical page numbers: CODE32 pages, CODE16 pages, then DATA;
  * underlying physical file starts with DATA, then CODE32, then CODE16. */
 for(i=0;i<pages;i++){
  unsigned own,logical=i;U32 bp=0x90U+pt+4U*i;
  if(obj_count==2){
   if(logical<opages[0])own=1+logical+opages[1];
   else own=1+logical-opages[0];
  }else{
   if(logical<opages[0])own=1+logical+opages[2];
   else if(logical<opages[0]+opages[1])own=1+logical+opages[2];
   else own=1+logical-(opages[0]+opages[1]);
  }
  buf[bp+2]=(U8)own;
 }
 for(i=0;i<=pages;i++)r4e_w32(buf+0x90U+pgfix+4U*i,offs[i]);
 memcpy(buf+0x90U+fxrec,rec,(size_t)recsize);
 {U32 pmod=0x90U+modtab;for(i=0;i<r4e_dll_count;i++){
  unsigned len=(unsigned)strlen(r4e_dll_names[i]);buf[pmod++]=(U8)len;memcpy(buf+pmod,r4e_dll_names[i],len);pmod+=len;}}
 /* Data physical pages are emitted as zeros beyond source-initialized bytes. */
 n=0;
 for(i=0;i<obj_count;i++){
  unsigned ob=(i==0?obj_count-1:i-1);
  for(j=0;j<opages[ob];j++){
   U32 remain=imgs[ob].length-j*R4E_PS;
   U32 take=remain<R4E_PS?remain:R4E_PS;
   /* BSS tail is excluded by r4e_page_count; all initialized pages are emitted. */
   memcpy(buf+datastart+n,imgs[ob].data+j*R4E_PS,(size_t)take);
   n+=(ob==obj_count-2 && j+1==opages[ob]?take:R4E_PS);
  }
 }
 filesize=datastart+n;
 f=fopen(output,"wb");if(!f){perror(output);free(buf);return 0;}
 ok=fwrite(buf,1,(size_t)filesize,f)==(size_t)filesize;if(fclose(f))ok=0;
 if(!ok){remove(output);fputs("R4E: output write failure\n",stderr);}
 else printf("R4E EXPERIMENTAL LE output=%s bytes=%lu objects=%u pages=%u fixups=%u+%u dlls=%u entry=%08lX scaffold=NO\n",output,(unsigned long)filesize,obj_count,pages,numint,numimp,r4e_dll_count,(unsigned long)entry);
 free(buf);return ok;
}
/* 0 = explicit legacy, 1 = choose from input bitness, 2 = force depth order.
 * This input-based policy is provisional for arbitrary untested OMF programs. */
static int r4l_order_policy=1;
static int r4l_has_16bit_code(const Members*ms){unsigned i,j;
 for(i=0;i<ms->n;i++)if(ms->v[i].selected){const Module*m=&ms->v[i].m;
  for(j=0;j<m->ns;j++)if(category(&m->seg[j])==0&&!m->seg[j].use32&&m->seg[j].length)return 1;
 }
 return 0;
}
int main(int argc,char**argv){Members ms;unsigned j;int i,ok=1,first=0;const char *output=NULL,*def=NULL;char dir[1024],*q;
 memset(&ms,0,sizeof ms);
 for(i=1;i<argc;i++){
  if(!strcmp(argv[i],"--omf-auto-order")){r4l_order_policy=1;continue;}
  if(!strcmp(argv[i],"--r4l-bfs-order")){r4l_order_policy=2;continue;}
  if(!strcmp(argv[i],"--legacy-order")){r4l_order_policy=0;continue;}
  if(!strcmp(argv[i],"-o")&&i+1<argc){output=argv[++i];continue;}
  if(!strcmp(argv[i],"-d")&&i+1<argc){def=argv[++i];continue;}
  if(!strncmp(argv[i],"--stack=",8)){char*end;unsigned long x=strtoul(argv[i]+8,&end,10);if(!*end&&x<=0xffffffffUL)override_stack=x;else ok=0;continue;}
  if(!first)first=i;
 }
 if(!ok||!output||!def||!first||argc-first<5||strlen(output)>900){
  fputs("usage: link386r4e -o TARGET.EXE -d TARGET.DEF [--stack=N] application.obj ... LIBC.LIB OS2386.LIB\n",stderr);return 2;
 }
 strcpy(dir,output);q=strrchr(dir,'/');if(!q)q=strrchr(dir,'\\');if(q)*q=0;else strcpy(dir,".");
 for(i=first;i<argc;i++)if(strcmp(argv[i],"--legacy-order")&&strcmp(argv[i],"--omf-auto-order")&&strcmp(argv[i],"--r4l-bfs-order")&&strncmp(argv[i],"--stack=",8)&&!input_file(&ms,argv[i]))ok=0;
 if(ok&&!r4e_def_imports(&ms,def))ok=0;
 if(ok){
  r4e_register_dll_("DOSCALLS");r4e_register_dll_("VIOCALLS");r4e_register_dll_("KBDCALLS");
  select_members(&ms);
  r4l_order_mode=(r4l_order_policy==2 || (r4l_order_policy==1 && !r4l_has_16bit_code(&ms)));
  printf("R4L POLICY %s (selected input has16=%d)\n",r4l_order_mode?"OMF_BFS_HASH":"STABLE_COMPAT",r4l_has_16bit_code(&ms));
  if(r4l_order_mode&&!r4l_assign_depths(&ms)){fputs("R4L depth computation failed\n",stderr);ok=0;}
  if(!place_segments(&ms)||!place_commons(&ms)||!combine(&ms))ok=0;
  else{make_layout_order();if(!assign_final())ok=0;
   else{
    if(obj_count==2){
     /* General C/386 flat LE rule: object bases occupy non-overlapping 64KiB
      * windows; the next base follows the actual preceding virtual extent. */
     if(obj_size[0]>0xffffffffUL-obj_base[0]-R4C_PREFIX ||
        !align_(obj_base[0]+obj_size[0]+R4C_PREFIX,65536UL,&obj_base[1])){
       fputs("R4L: virtual base overflow\n",stderr);ok=0;
     }
     /* Microsoft aligns the code-object logical extent to a DWORD. */
     obj_size[0]=(obj_size[0]+3UL)&~3UL;
     /* Rebuild imports in source-discovery order without CMD32-only DLLs. */
     r4e_dll_count=0;
    }
    if(!images_build(&ms)||!apply_(&ms,dir))ok=0;
    else{
     if(obj_count==2){unsigned ix;for(ix=0;ix<obj_count;ix++)if(!save_image(dir,ix,"r4f-mask",imgs[ix].mask,imgs[ix].length))ok=0;}
     if(ok&&!r4e_emit(output,&ms))ok=0;
    }
   }}
 }
 for(j=0;j<obj_count;j++){free(imgs[j].data);free(imgs[j].mask);}for(j=0;j<ms.n;j++){free(ms.v[j].file);module_free(&ms.v[j].m);}free(ms.v);
 return ok?0:1;
}
