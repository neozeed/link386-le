/* LINK386 R4C: independent two-object source image construction.
 * Uses selected hi.obj / LIBC.LIB / OS2386.LIB only, never reference hi.exe.
 * R3G's OMF SEGDEF/GRPDEF combinator supplies region placements.
 * 16-byte code-object prefix is the explicitly bounded C/386 hello profile.
 * Produces UNRELOCATED images and masks, NOT a runnable LE.
 * Strict ISO C89; all segment and fixup site bounds checked.
 */
#define R4D_LIDATA_FIXUPS
#define R3N_EMBED
#include "link386r4c_layout_core.inc"
#define R4C_PREFIX 16UL
#define MAX_OUT (16UL*1024UL*1024UL)
typedef struct{U8*data,*mask;U32 length;} Image;
static Image imgs[R3M_MAX_OBJECTS];
static U32 r4d_fixsites[R4D_FIXUP_SITE_CAP];
static int save_image(const char *dir,unsigned ob,const char*suffix,const U8*p,U32 len){char path[1024];FILE*f;int ok;
 if(strlen(dir)+50>=sizeof(path))return 0;
 sprintf(path,"%s/object%u-%s.bin",dir,ob+1,suffix);f=fopen(path,"wb");if(!f){perror(path);return 0;}
 ok=fwrite(p,1,(size_t)len,f)==(size_t)len;if(fclose(f))ok=0;if(!ok){remove(path);fprintf(stderr,"R4C: output write failure %s\n",path);}return ok;}
static int images_build(const Members*ms){unsigned i,j,files=0,marked=0,fixes=0,conflicts=0,selected=0;U32 off,size;
 for(i=0;i<obj_count;i++){size=obj_size[i]+(i==0?R4C_PREFIX:0);if(size>MAX_OUT||size<obj_size[i])return 0;imgs[i].length=size;imgs[i].data=(U8*)calloc((size_t)size?size:1,1);imgs[i].mask=(U8*)calloc((size_t)size?size:1,1);if(!imgs[i].data||!imgs[i].mask)return 0;}
 for(i=0;i<ms->n;i++)if(ms->v[i].selected)selected++;
 for(i=0;i<cn;i++){const Contribution*c=&contrib[i];const Module*m=&ms->v[c->member].m;unsigned obj=finalobj[placed[i].out];Image*im=&imgs[obj];
  off=finalseg[placed[i].out]+placed[i].offset+(obj==0?R4C_PREFIX:0);
  if(off>im->length||c->size>im->length-off)return 0;
  if(c->size)printf("R4C PLACE module=%s seg=%s group=%s object=%u offset=%08lX size=%lu\n",m->name,m->seg[c->seg-1].name,group_name(m,&m->seg[c->seg-1]),obj+1,(unsigned long)off,(unsigned long)c->size);
  for(j=0;j<m->nd;j++)if(m->data[j].seg==c->seg){const Data*d=&m->data[j];size_t v;
   if(d->offset>c->size||d->length>c->size-d->offset)return 0;
   /* OMF COMMON combine=6 overlays contributions: the longest one is
      authoritative for this verified hello profile (e.g. 19-byte PAD
      replaces several four-byte FF PAD contributions). */
   if(m->seg[c->seg-1].combine==6 && c->size<os[placed[i].out].size)continue;
   for(v=0;v<d->length;v++){U32 at=off+d->offset+(U32)v;if(im->mask[at]&&im->data[at]!=d->bytes[v]){fprintf(stderr,"R4C: overlapping OMF contributions: %s seg %u site %08lX\n",m->name,c->seg,(unsigned long)at);conflicts++;return 0;}
   im->data[at]=d->bytes[v];im->mask[at]=1;marked++;}files++;
  }
  for(j=0;j<m->nf;j++)if(m->fix[j].seg==c->seg){const Fixup*f=&m->fix[j];const Data*d=f->data_id&&f->data_id<=m->nd?&m->data[f->data_id-1]:NULL;unsigned width,k2,site_count,si;U32 at;
   if(!d)return 0;
   if(f->loc==9||f->loc==11)width=4;else if(f->loc==1||f->loc==2||f->loc==3)width=2;else if(f->loc==0)width=1;else {fprintf(stderr,"R4C: unknown loc type %u\n",f->loc);return 0;}
   if(!fixup_source_sites(d,f->at,width,r4d_fixsites,R4D_FIXUP_SITE_CAP,&site_count))return 0;
   for(si=0;si<site_count;si++){
    if(d->offset>c->size||r4d_fixsites[si]>c->size-d->offset||width>c->size-d->offset-r4d_fixsites[si])return 0;
    at=off+d->offset+r4d_fixsites[si];for(k2=0;k2<width;k2++)im->mask[at+k2]=2;
   }fixes+=site_count;
  }
 }
 for(i=0;i<obj_count;i++){U32 regular=0,reloc=0,missing=0,q;for(q=0;q<imgs[i].length;q++){if(imgs[i].mask[q]==1)regular++;else if(imgs[i].mask[q]==2)reloc++;else missing++;}printf("R4C OBJECT object=%u base=%08lX size=%lu mask_regular=%lu mask_fixup=%lu mask_unsourced=%lu\n",i+1,(unsigned long)obj_base[i],(unsigned long)imgs[i].length,(unsigned long)regular,(unsigned long)reloc,(unsigned long)missing);}
 printf("R4C IMAGES selected=%u contributions=%u data_records=%u initialized_stores=%u fixups=%u write_conflicts=%u reference_used=NO relocation_writebacks=0 runnable_le=NO\n",selected,cn,files,marked,fixes,conflicts);
 return 1;
}

#ifndef R4C_LIBRARY_BUILD
int main(int argc,char **argv){Members ms;int i,ok=1;unsigned j;char *dir;memset(&ms,0,sizeof(ms));
 if(argc!=5){fprintf(stderr,"usage: link386r4c_images EXISTING_OUTPUT_DIRECTORY hi.obj LIBC.LIB OS2386.LIB\n");return 2;}
 dir=argv[1];if(strlen(dir)>900){fprintf(stderr,"R4C: directory too long\n");return 2;}
 for(i=2;i<5;i++)if(!input_file(&ms,argv[i]))ok=0;
 if(ok){select_members(&ms);if(!place_segments(&ms)||!place_commons(&ms)||!combine(&ms))ok=0;else{make_layout_order();if(!assign_final()||!images_build(&ms))ok=0;}}
 if(ok){for(j=0;j<obj_count;j++){if(!save_image(dir,j,"unrelocated",imgs[j].data,imgs[j].length)||!save_image(dir,j,"mask",imgs[j].mask,imgs[j].length)){ok=0;break;}}}
 for(j=0;j<obj_count;j++){free(imgs[j].data);free(imgs[j].mask);}for(j=0;j<ms.n;j++){free(ms.v[j].file);module_free(&ms.v[j].m);}free(ms.v);return ok?0:1;}

#endif /* R4C_LIBRARY_BUILD */
