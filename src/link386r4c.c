/* LINK386 R4C - independent Microsoft OMF contribution materializer.
 * Builds each selected OBJ/LIBC/OS2386 segment's initialized bytes strictly
 * from OMF LEDATA/LIDATA, with no Microsoft LE input or byte borrowing.
 * It does NOT yet perform cross-module relocation or write a runnable LE.
 * ISO C89; bounded allocations; rejects inconsistent overlapping source data.
 */
#define main r2_internal_main
#include "link386r2.c"
#undef main
#include <errno.h>

#define MAX_SEGMENT (16UL*1024UL*1024UL)
static U32 crc32c(const U8*p,size_t n){U32 c=0xffffffffUL;size_t i;unsigned j;for(i=0;i<n;i++){c^=p[i];for(j=0;j<8;j++)c=(c&1)?(c>>1)^0xedb88320UL:c>>1;}return (c^0xffffffffUL)&0xffffffffUL;}
static int emit(const char *path,const U8*p,size_t n){FILE*f=fopen(path,"wb");int ok;if(!f){perror(path);return 0;}ok=fwrite(p,1,n,f)==n;if(fclose(f))ok=0;if(!ok){fprintf(stderr,"R4C: write failure: %s\n",path);remove(path);}return ok;}
/* No C99 snprintf under strict C89: construct bounded paths with sprintf only
 * after checking length for fixed-size decimal suffix. */
static int path_(char *dst,size_t cap,const char*dir,unsigned mi,unsigned si){size_t n=strlen(dir);if(n+38>=cap||mi>9999||si>99)return 0;sprintf(dst,"%s/module-%04u-seg-%02u.bin",dir,mi,si);return 1;}
static const char* methodname(unsigned x){switch(x){case 0:return "SEG";case 1:return "GRP";case 2:return "EXT";default:return "OTHER";}}
int main(int argc,char **argv){Members ms;FILE *inventory=NULL,*fx=NULL,*symbols=NULL;int ok=1;unsigned i,j,k,selected=0,segments=0,datacount=0,bytes=0,fixcount=0,imports=0,issue=0,collisions=0;const char*dir;char path[1024];memset(&ms,0,sizeof(ms));
 if(argc!=5){fprintf(stderr,"usage: link386r4c OUT_DIRECTORY hi.obj LIBC.LIB OS2386.LIB\n(note: OUT_DIRECTORY must exist)\n");return 2;}
 dir=argv[1];if(strlen(dir)>900){fprintf(stderr,"R4C: output path too long\n");return 2;}
 for(i=2;i<5;i++)if(!input_file(&ms,argv[i]))ok=0;
 if(!ok)goto cleanup;
 select_members(&ms);
 sprintf(path,"%s/contributions.tsv",dir);inventory=fopen(path,"wb");sprintf(path,"%s/fixups.tsv",dir);fx=fopen(path,"wb");sprintf(path,"%s/symbols.tsv",dir);symbols=fopen(path,"wb");
 if(!inventory||!fx||!symbols){fprintf(stderr,"R4C: cannot open report output files\n");ok=0;goto cleanup;}
 fputs("member\tmodule\tlibrary\tseg\tname\tclass\tgroup\tuse32\tcombine\talign\tsize\tdata_records\tdata_bytes\tcrc32\tfile\n",inventory);
 fputs("member\tmodule\tseg\tsegment_name\tdata_id\tdata_offset\tsite_within_data\tsegment_site\tloc\tmode\tframe_method\tframe_idx\ttarget_method\ttarget_idx\ttarget_symbol\tdisplacement\tstatus\n",fx);
 fputs("member\tmodule\tkind\tname\tseg\tvalue\tadditional\n",symbols);
 for(i=0;i<ms.n;i++)if(ms.v[i].selected){const Module*m=&ms.v[i].m;const char*mn=m->name?m->name:"?";selected++;
  for(j=0;j<m->ns;j++){const Segment*s=&m->seg[j];U8*raw,*used;unsigned nd=0;U32 written=0;if(s->length>MAX_SEGMENT){fprintf(stderr,"R4C: oversized segment %s/%s\n",mn,s->name);ok=0;goto cleanup;}
   raw=(U8*)calloc(s->length?(size_t)s->length:1,1);used=(U8*)calloc(s->length?(size_t)s->length:1,1);if(!raw||!used){free(raw);free(used);ok=0;goto cleanup;}
   for(k=0;k<m->nd;k++)if(m->data[k].seg==j+1){const Data*d=&m->data[k];size_t u;nd++;datacount++;if(d->offset>s->length||d->length>s->length-d->offset){fprintf(stderr,"R4C: data exceeds declared segment: %s/%s at %lu + %lu > %lu\n",mn,s->name,(unsigned long)d->offset,(unsigned long)d->length,(unsigned long)s->length);free(raw);free(used);ok=0;goto cleanup;}
    for(u=0;u<d->length;u++){size_t at=(size_t)d->offset+u;if(used[at]&&raw[at]!=d->bytes[u]){fprintf(stderr,"R4C: conflicting data records in %s/%s offset %lu\n",mn,s->name,(unsigned long)at);collisions++;free(raw);free(used);ok=0;goto cleanup;}raw[at]=d->bytes[u];if(!used[at]){used[at]=1;written++;}}
   }
   if(!path_(path,sizeof(path),dir,i,j+1)){free(raw);free(used);ok=0;goto cleanup;}
   if(s->length&&!emit(path,raw,(size_t)s->length)){free(raw);free(used);ok=0;goto cleanup;}
   fprintf(inventory,"%u\t%s\t%s\t%u\t%s\t%s\t%s\t%u\t%u\t%u\t%lu\t%u\t%lu\t%08lX\t%s\n",i,mn,ms.v[i].islib?"LIB":"OBJ",j+1,s->name,s->class_name,(s->group&&s->group<=m->ng)?m->grp[s->group-1].name:"-",s->use32,s->combine,s->align,(unsigned long)s->length,nd,(unsigned long)written,(unsigned long)crc32c(raw,(size_t)s->length),s->length?path:"-");
   bytes+=(unsigned)written;segments++;free(raw);free(used);
  }
  for(j=0;j<m->np;j++){const Public*p=&m->pub[j];fprintf(symbols,"%u\t%s\tPUB\t%s\t%u\t%lu\t%u\n",i,mn,p->name,p->seg,(unsigned long)p->offset,p->grp);}
  for(j=0;j<m->nc;j++){const Communal*c=&m->com[j];fprintf(symbols,"%u\t%s\tCOM\t%s\t0\t%lu\t%u\n",i,mn,c->name,(unsigned long)c->size,c->local);}
  for(j=0;j<m->ni;j++){const Import*p=&m->imports[j];fprintf(symbols,"%u\t%s\tIMPORT\t%s\t0\t%u\t%s.%s\n",i,mn,p->internal,p->byordinal?p->ordinal:0,p->dll,p->byordinal?"ordinal":p->symbol);imports++;}
  for(j=0;j<m->nf;j++){const Fixup*f=&m->fix[j];const Data*d=(f->data_id&&f->data_id<=m->nd)?&m->data[f->data_id-1]:NULL;const char *status="SOURCE_OK";const char*target="-";U32 site=0;unsigned width=0;fixcount++;
   if(f->target_method==2&&f->target_idx&&f->target_idx<=m->ne)target=m->ext[f->target_idx-1].name;
   if(f->loc==9||f->loc==11)width=4;else if(f->loc==1||f->loc==2||f->loc==3)width=2;else if(f->loc==0)width=1;
   if(d){site=d->offset+f->at;if(!f->seg||f->seg>m->ns||site>m->seg[f->seg-1].length||width>m->seg[f->seg-1].length-site){status="BAD_SEGMENT_SITE";issue++;}
    else if(f->at>d->length||width>d->length-f->at){status="CROSSES_DATA_RECORD";issue++;}}
   else{status="NO_SOURCE_RECORD";issue++;}
   if(f->target_method==2&&(!f->target_idx||f->target_idx>m->ne)){status="EXTERNAL_INDEX_OUT_OF_RANGE";issue++;}
   fprintf(fx,"%u\t%s\t%u\t%s\t%u\t%lu\t%u\t%lu\t%u\t%u\t%u\t%u\t%s\t%u\t%s\t%08lX\t%s\n",i,mn,f->seg,(f->seg&&f->seg<=m->ns)?m->seg[f->seg-1].name:"?",f->data_id,d?(unsigned long)d->offset:0,f->at,(unsigned long)site,f->loc,f->mode,f->frame_method,f->frame_idx,methodname(f->target_method),f->target_idx,target,(unsigned long)f->disp,status);
  }
 }
 printf("R4C MATERIALIZED selected=%u segments=%u data_records=%u initialized_bytes=%u fixups=%u imports=%u issues=%u conflicts=%u reference_exe_used=NO runnable_le_emitted=NO\n",selected,segments,datacount,bytes,fixcount,imports,issue,collisions);
 cleanup:if(inventory)fclose(inventory);if(fx)fclose(fx);if(symbols)fclose(symbols);for(i=0;i<ms.n;i++){free(ms.v[i].file);module_free(&ms.v[i].m);}free(ms.v);return ok?0:1;}
