/* LINK386 R3: conservative OMF32 -> LE image generator. ISO C89. */
#define main r2_internal_main
#include "link386r2.c"
#undef main
#include <errno.h>
#define MAX_OBJECTS 128
#define MAX_PAGES 2048
#define PAGE_BYTES 4096UL
#define HEADER_POS 0x80UL
#define HEADER_BYTES 0xb0UL
#define MAX_IMAGE (16UL*1024UL*1024UL)
typedef struct { const Module *m; const Segment *s; unsigned seg; U32 base, pages, page_index; U8 *bytes; } Object;
static void w16(U8 *p,U32 v){p[0]=(U8)v;p[1]=(U8)(v>>8);}
static void w32(U8 *p,U32 v){w16(p,v);w16(p+2,v>>16);}
static U32 r32(const U8 *p){return (U32)p[0]|((U32)p[1]<<8)|((U32)p[2]<<16)|((U32)p[3]<<24);}
static U32 align_up(U32 n,U32 a){return (n+a-1)&~(a-1);}
static int fail_(const char *why){fprintf(stderr,"LINK386 R3: %s\n",why);return 0;}
static int build(const Members *ms,const char *out,const char *entry){Object obj[MAX_OBJECTS];unsigned no=0,pages=0,mi,si,j,k,entry_obj=0;U32 entry_off=0,objects_off,map_off,fixpage_off,fixrecord_off,loader_end,data_off,total,phys=0;U8 *image;FILE *fp;int ok=0;
 memset(obj,0,sizeof(obj));
 for(mi=0;mi<ms->n;mi++)if(ms->v[mi].selected){const Module *m=&ms->v[mi].m;
  if(m->ni)return fail_("import fixup generation is not implemented; refusing potentially invalid LE");
  for(j=0;j<m->ne;j++)if(!selected_export(ms,m->ext[j].name)){fprintf(stderr,"LINK386 R3: unresolved external %s\n",m->ext[j].name);return 0;}
  for(si=0;si<m->ns;si++) {const Segment *s=&m->seg[si];Object *o; if(!s->use32)return fail_("16-bit segment unsupported");if(no>=MAX_OBJECTS-1)return fail_("too many objects");o=&obj[no++];o->m=m;o->s=s;o->seg=si+1;o->base=(U32)(no)*0x10000UL;o->pages=(s->length+PAGE_BYTES-1)/PAGE_BYTES;o->page_index=pages+1;
   if(s->length>MAX_IMAGE||o->pages>MAX_PAGES-pages)return fail_("segment/page size limit exceeded");
   pages+=(unsigned)o->pages;
   o->bytes=(U8*)calloc((size_t)(o->pages ? o->pages*PAGE_BYTES : 1),1);if(!o->bytes)return fail_("out of memory");
   for(j=0;j<m->nd;j++)if(m->data[j].seg==si+1){const Data *d=&m->data[j];if(d->offset>s->length||d->length>s->length-d->offset)return fail_("LEDATA extends beyond segment");memcpy(o->bytes+d->offset,d->bytes,d->length);}
  }
 }
 /* R3A: apply only provably intra-object 32-bit OMF fixes. No LE runtime fixups. */
 for(mi=0;mi<ms->n;mi++)if(ms->v[mi].selected){const Module *m=&ms->v[mi].m;
  for(j=0;j<m->nf;j++){const Fixup *f=&m->fix[j];const Data *d;Object *o=NULL;U32 site,target,addend,value;
   if(!f->data_id||f->data_id>m->nd)return fail_("invalid fixup data reference");
   d=&m->data[f->data_id-1];site=d->offset+f->at;
   for(k=0;k<no;k++)if(obj[k].m==m&&obj[k].seg==f->seg){o=&obj[k];break;}
   if(!o||f->loc!=9||site>o->s->length||o->s->length-site<4||f->at>d->length||d->length-f->at<4)return fail_("unsupported or out-of-range fixup");
   if(f->target_method!=0||f->target_idx!=f->seg)return fail_("cross-object/external fixup requires LE relocation support");
   /* The encoded displacement and original field addend both contribute. */
   target=f->has_disp?f->disp:0;addend=r32(o->bytes+site);
   if(f->mode){value=target+addend;}else{value=target+addend-(site+4UL);}
   w32(o->bytes+site,value);
  }
 }
 if(!no)return fail_("no 32-bit segments to link");
 for(mi=0;mi<ms->n;mi++)if(ms->v[mi].selected){const Module *m=&ms->v[mi].m;for(j=0;j<m->np;j++)if(!strcmp(m->pub[j].name,entry)) {if(entry_obj)return fail_("duplicate entry symbol");for(k=0;k<no;k++)if(obj[k].m==m&&obj[k].seg==m->pub[j].seg){entry_obj=k+1;entry_off=m->pub[j].offset;if(entry_off>=obj[k].s->length)return fail_("entry beyond segment");break;}}}
 if(!entry_obj)return fail_("entry symbol was not found; use -e NAME");
 if(no>=MAX_OBJECTS)return fail_("object limit exceeded");
 /* one extra RW stack object, zero file pages */
 objects_off=HEADER_BYTES;map_off=objects_off+(no+1)*24UL;fixpage_off=map_off+pages*4UL+2UL;fixrecord_off=fixpage_off+(pages+1UL)*4UL;loader_end=fixrecord_off;data_off=align_up(HEADER_POS+loader_end,512UL);total=data_off+pages*PAGE_BYTES;
 if(total>MAX_IMAGE)return fail_("output image too large");
 image=(U8*)calloc((size_t)total,1);if(!image)return fail_("out of memory");
 image[0]='M';image[1]='Z';w16(image+2,0x40);w16(image+4,1);w16(image+8,4);w16(image+0x18,0x40);w32(image+0x3c,HEADER_POS);
 {U8 *h=image+HEADER_POS;h[0]='L';h[1]='E';w16(h+8,2);w16(h+10,1);w32(h+0x10,0x200UL);w32(h+0x14,pages);w32(h+0x18,entry_obj);w32(h+0x1c,entry_off);w32(h+0x20,no+1);w32(h+0x24,0x10000);w32(h+0x28,PAGE_BYTES);w32(h+0x2c,PAGE_BYTES);w32(h+0x30,(pages+1UL)*4UL);w32(h+0x38,loader_end-HEADER_BYTES);w32(h+0x40,objects_off);w32(h+0x44,no+1);w32(h+0x48,map_off);w32(h+0x58,map_off+pages*4UL);w32(h+0x5c,map_off+pages*4UL+1);w32(h+0x68,fixpage_off);w32(h+0x6c,fixrecord_off);w32(h+0x70,fixrecord_off);w32(h+0x78,fixrecord_off);w32(h+0x80,data_off);w32(h+0x84,pages);w32(h+0x94,no+1);w32(h+0xac,0x10000);
 for(k=0;k<no;k++){Object *o=&obj[k];U8 *r=h+objects_off+k*24UL;U32 fl=0x0003UL; /* read/write; code below */
 if(strstr(o->s->name,"TEXT")||strstr(o->s->name,"CODE"))fl=0x0005UL;else if(strstr(o->s->name,"CONST"))fl=0x0001UL;
 w32(r,o->s->length);w32(r+4,o->base);w32(r+8,fl);w32(r+12,o->pages?o->page_index:0);w32(r+16,o->pages);
 for(j=0;j<o->pages;j++){U8 *pm=h+map_off+phys*4UL;U32 pn=phys+1;pm[0]=(U8)(pn>>16);pm[1]=(U8)(pn>>8);pm[2]=(U8)pn;pm[3]=0;memcpy(image+data_off+phys*PAGE_BYTES,o->bytes+j*PAGE_BYTES,PAGE_BYTES);phys++;}
 }
 {U8 *r=h+objects_off+no*24UL;w32(r,0x10000);w32(r+4,(no+1UL)*0x10000UL);w32(r+8,3);}
 h[map_off+pages*4UL]=0; /* null resident name list */
 }
 fp=fopen(out,"wb");if(!fp){fprintf(stderr,"%s: %s\n",out,strerror(errno));goto cleanup;}if(fwrite(image,1,(size_t)total,fp)==(size_t)total&&fclose(fp)==0){fp=NULL;ok=1;printf("LINK386 R3 LE EMITTED %s objects=%u pages=%u bytes=%lu entry=%u:%08lX\n",out,no+1,pages,(unsigned long)total,entry_obj,(unsigned long)entry_off);}else{fclose(fp);fp=NULL;remove(out);fail_("write failed");}
 cleanup:free(image);for(k=0;k<no;k++)free(obj[k].bytes);return ok;
}
int main(int argc,char **argv){Members ms;int i,ok=1,files=0;const char *out=NULL,*entry=NULL;unsigned j;memset(&ms,0,sizeof(ms));for(i=1;i<argc;i++){if(!strcmp(argv[i],"-o")&&i+1<argc)out=argv[++i];else if(!strcmp(argv[i],"-e")&&i+1<argc)entry=argv[++i];else if(argv[i][0]=='-'){fprintf(stderr,"unknown option %s\n",argv[i]);ok=0;}else{files++;if(!input_file(&ms,argv[i]))ok=0;}}
 if(!files||!out||!entry){fprintf(stderr,"usage: link386r3 -o output.exe -e _entry file.obj [file.lib ...]\n");ok=0;}
 if(ok){select_members(&ms);ok=build(&ms,out,entry);}
 for(j=0;j<ms.n;j++){free(ms.v[j].file);module_free(&ms.v[j].m);}free(ms.v);return ok?0:1;}
