/* LINK386 R2: link-plan OMF reader. ISO C89; no LE emission. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
typedef unsigned char U8;
typedef unsigned long U32;
typedef struct { const U8 *p; size_t n, i; int bad; } Reader;
typedef struct { char **v; unsigned n, cap; } Names;
typedef struct { char *name,*class_name; U32 length, base; unsigned align, combine, use32, group; } Segment;
typedef struct { char *name; unsigned *segments,n,cap; } Group;
typedef struct { char *name; unsigned seg,grp,local; U32 offset; } Public;
typedef struct { char *name; unsigned type,local; } External;
typedef struct { char *name; U32 size; unsigned extidx, local; } Communal;
typedef struct { char *internal,*dll,*symbol; unsigned byordinal,ordinal; } Import;
typedef struct { unsigned seg; U32 offset; size_t length; U8 *bytes; U8 *encoded; size_t encoded_len; unsigned countsize; } Data;
typedef struct { unsigned seg, at, loc, mode, frame_method, frame_idx, target_method,target_idx, has_disp; U32 disp; unsigned data_id; } Fixup;
typedef struct { unsigned method,idx; } Thread;
typedef struct { char *name; Segment *seg; unsigned ns,cs; Group *grp; unsigned ng,cg; Public *pub; unsigned np,cp; External *ext; unsigned ne,ce; Communal *com; unsigned nc,cc; Import *imports; unsigned ni,ci; Data *data; unsigned nd,cd; Fixup *fix; unsigned nf,cf; Thread frame[4],target[4]; unsigned last_data; Names lnames; } Module;
typedef struct { char *file; unsigned long offset; Module m; int selected, islib; unsigned selection_rank; } Member;
typedef struct { Member *v; unsigned n,cap; } Members;
static unsigned rd(Reader *r){if(r->i>=r->n){r->bad=1;return 0;}return r->p[r->i++];}
static unsigned ix(Reader *r){unsigned x=rd(r);if(x&0x80)x=((x&127)<<8)|rd(r);return x;}
static U32 num(Reader *r,unsigned n){U32 v=0;unsigned i;for(i=0;i<n;i++)v|=((U32)rd(r))<<(8*i);return v;}
static char *dup_(const char *s){size_t n=strlen(s)+1;char *p=(char*)malloc(n);if(p)memcpy(p,s,n);return p;}
static char *string_(Reader *r){unsigned n=rd(r);char *s;if(r->bad||n>r->n-r->i){r->bad=1;return NULL;}s=(char*)malloc((size_t)n+1);if(!s){r->bad=1;return NULL;}memcpy(s,r->p+r->i,n);s[n]=0;r->i+=n;return s;}
static int grow(void **ptr,unsigned *cap,unsigned needed,size_t unit){void *p;unsigned n=*cap;if(needed<=n)return 1;n=n?n*2:8;while(n<needed){if(n>UINT_MAX/2)return 0;n*=2;}if((size_t)n>((size_t)-1)/unit)return 0;p=realloc(*ptr,(size_t)n*unit);if(!p)return 0;*ptr=p;*cap=n;return 1;}
static const char *nm(const Names *n,unsigned i){return i&&i<=n->n?n->v[i-1]:"?";}
static int addname(Names *n,char *s){if(!s||!grow((void**)&n->v,&n->cap,n->n+1,sizeof(char*)))return 0;n->v[n->n++]=s;return 1;}
static void free_names(Names *n){unsigned i;for(i=0;i<n->n;i++)free(n->v[i]);free(n->v);memset(n,0,sizeof(*n));}
static void module_free(Module *m){unsigned i;free(m->name);for(i=0;i<m->ns;i++){free(m->seg[i].name);free(m->seg[i].class_name);}for(i=0;i<m->ng;i++){free(m->grp[i].name);free(m->grp[i].segments);}for(i=0;i<m->np;i++)free(m->pub[i].name);for(i=0;i<m->ne;i++)free(m->ext[i].name);for(i=0;i<m->nc;i++)free(m->com[i].name);for(i=0;i<m->ni;i++){free(m->imports[i].internal);free(m->imports[i].dll);free(m->imports[i].symbol);}for(i=0;i<m->nd;i++){free(m->data[i].bytes);free(m->data[i].encoded);}free(m->seg);free(m->grp);free(m->pub);free(m->ext);free(m->com);free(m->imports);free(m->data);free(m->fix);free_names(&m->lnames);memset(m,0,sizeof(*m));}
/* Iterated data: count is 16/32-bit, followed by block count.  Zero
 * block count means a length-prefixed literal; otherwise recursively
 * expand the nested blocks.  Keep a strict expansion limit and depth. */
static int iter_append(U8 **buf,size_t *used,size_t *cap,const U8 *p,size_t n){
 size_t want;U8 *q;size_t c;
 if(n>16UL*1024UL*1024UL-*used)return 0;
 want=*used+n;
 if(want>*cap){c=*cap?*cap:64;while(c<want){if(c>16UL*1024UL*1024UL/2){c=16UL*1024UL*1024UL;break;}c*=2;}
 q=(U8*)realloc(*buf,c);if(!q)return 0;*buf=q;*cap=c;}
 if(n){memcpy(*buf+*used,p,n);}*used=want;return 1;
}
static int iter_block(Reader *r,unsigned countsize,unsigned depth,U8 **buf,size_t *used,size_t *cap){
 U32 reps;unsigned blocks,i;U32 j;size_t start,end,n;
 if(depth>32)return 0;
 reps=num(r,countsize);blocks=(unsigned)num(r,2);if(r->bad)return 0;
 start=*used;
 if(blocks==0){n=rd(r);if(r->bad||n>r->n-r->i)return 0;
 if(!iter_append(buf,used,cap,r->p+r->i,n)){return 0;}r->i+=n;
 }else for(i=0;i<blocks;i++)if(!iter_block(r,countsize,depth+1,buf,used,cap))return 0;
 end=*used;n=end-start;
 if(n && reps && (unsigned long)reps>(16UL*1024UL*1024UL-*used)/n+1UL)return 0;
 if(reps==0){*used=start;return 1;}
 if(reps>1 && n){
  /* Repetition source must not alias an allocation that iter_append may
     realloc.  Earlier R2 copied from freed storage here, making certain
     LIDATA32 literal repetitions nondeterministic across processes. */
  U8 *seed=(U8*)malloc(n);
  if(!seed)return 0;
  memcpy(seed,*buf+start,n);
  for(j=1;j<reps;j++)if(!iter_append(buf,used,cap,seed,n)){free(seed);return 0;}
  free(seed);
 }
 return 1;
}
/* OMF COMDEF numeric leaves: <=0x80 inline, 0x81 16-bit,
 * 0x84 24-bit, 0x88 32-bit. Reject unknown tags and overflow. */
static U32 com_number(Reader *r){
 unsigned c=rd(r);if(c<=0x80)return c;
 if(c==0x81)return num(r,2);
 if(c==0x84)return num(r,3);
 if(c==0x88)return num(r,4);
 r->bad=1;return 0;
}
#ifdef R4D_LIDATA_FIXUPS
/* R4D: FIXUPP source locations for LIDATA address bytes in the *encoded*
 * data stream.  Expand the referenced literal position to every logical
 * occurrence, including nested/repeated blocks.  No speculative cross-literal
 * writes.  All returned offsets are relative to the LIDATA contribution. */
#define R4D_FIXUP_SITE_CAP 8192U
static int lidata_walk(Reader *r,unsigned cs,unsigned depth,unsigned loc,unsigned width,
 U32 *sites,unsigned cap,unsigned *count,U32 *outpos){
 U32 reps,unit,base,delta;unsigned blocks,i,first,newcount;size_t literal;unsigned len;
 if(depth>32)return 0;
 reps=num(r,cs);blocks=(unsigned)num(r,2);if(r->bad)return 0;
 base=*outpos;first=*count;
 if(!blocks){
  len=rd(r);literal=r->i;
  if(r->bad || len>r->n-r->i)return 0;
  if((size_t)loc>=literal && (size_t)loc<literal+len){
   if(width>literal+len-(size_t)loc || *count>=cap)return 0;
   sites[(*count)++]=base+(U32)((size_t)loc-literal);
  }
  r->i+=len;if((U32)len>0xffffffffUL-*outpos)return 0;*outpos+=len;
 }else{for(i=0;i<blocks;i++)if(!lidata_walk(r,cs,depth+1,loc,width,sites,cap,count,outpos))return 0;}
 unit=*outpos-base;newcount=*count;
 if(reps==0){*count=first;*outpos=base;return 1;}
 if(unit && reps>(0xffffffffUL-base)/unit)return 0;
 if(reps>1){U32 q;for(q=1;q<reps;q++)for(i=first;i<newcount;i++){
  if(*count>=cap)return 0;
  delta=unit*q;sites[(*count)++]=sites[i]+delta;
 }}
 *outpos=base+unit*reps;
 return 1;
}
static int fixup_source_sites(const Data *d,unsigned loc,unsigned width,
 U32 *sites,unsigned cap,unsigned *count){
 Reader r;U32 expanded=0;*count=0;
 if(!d||!width)return 0;
 if(!d->encoded){if(loc>d->length||width>d->length-loc||!cap)return 0;sites[0]=loc;*count=1;return 1;}
 r.p=d->encoded;r.n=d->encoded_len;r.i=0;r.bad=0;
 while(r.i<r.n)if(!lidata_walk(&r,d->countsize,0,loc,width,sites,cap,count,&expanded))return 0;
 return !r.bad && r.i==r.n && expanded==d->length && *count>0;
}
#endif /* R4D_LIDATA_FIXUPS */
static int parse_record(Module *m,unsigned t,Reader *r){unsigned a,b,c,i;U32 off;char *s;Segment *seg;Group *grp;Public *pub;External *ext;Data *d;
 if(t==0x80||t==0x82){s=string_(r);if(!s)return 0;if(m->name)free(s);else m->name=s;}
 else if(t==0x96){while(r->i<r->n&&!r->bad){s=string_(r);if(!addname(&m->lnames,s)){free(s);return 0;}}}
 else if(t==0x98||t==0x99){a=rd(r);if((a>>5)==0)(void)num(r,2);off=num(r,(t&1)?4:2);b=ix(r);c=ix(r);(void)ix(r);if(!grow((void**)&m->seg,&m->cs,m->ns+1,sizeof(Segment)))return 0;seg=&m->seg[m->ns++];memset(seg,0,sizeof(*seg));seg->name=dup_(nm(&m->lnames,b));seg->class_name=dup_(nm(&m->lnames,c));if(!seg->name||!seg->class_name)return 0;seg->length=off;seg->align=a>>5;seg->combine=(a>>2)&7;seg->use32=a&1; /* OMF ACBP P bit, independent of 16/32-bit SEGDEF length record */}
 else if(t==0x9a||t==0x9b){a=ix(r);if(!grow((void**)&m->grp,&m->cg,m->ng+1,sizeof(Group)))return 0;grp=&m->grp[m->ng++];memset(grp,0,sizeof(*grp));grp->name=dup_(nm(&m->lnames,a));if(!grp->name)return 0;while(r->i<r->n&&!r->bad){a=rd(r);if(a==0xff){b=ix(r);if(!grow((void**)&grp->segments,&grp->cap,grp->n+1,sizeof(unsigned)))return 0;grp->segments[grp->n++]=b;}else { /* group component alternatives: external/name */ (void)ix(r); }}for(i=0;i<grp->n;i++)if(grp->segments[i]&&grp->segments[i]<=m->ns)m->seg[grp->segments[i]-1].group=m->ng;}
 else if(t==0x8c||t==0xb4||t==0xb5){while(r->i<r->n&&!r->bad){s=string_(r);a=ix(r);if(!s)return 0;if(!grow((void**)&m->ext,&m->ce,m->ne+1,sizeof(External))){free(s);return 0;}ext=&m->ext[m->ne++];ext->name=s;ext->type=a;ext->local=(t==0xb4||t==0xb5);}}
 else if(t==0xb0||t==0xb8){while(r->i<r->n&&!r->bad){
 U32 length=0,count=0;unsigned dtype,local=(t==0xb8);Communal *co;
 s=string_(r);(void)ix(r);dtype=rd(r);
 if(!s||r->bad){free(s);return 0;}
 if(dtype==0x62){length=com_number(r);}
 else if(dtype==0x61){count=com_number(r);length=com_number(r);
  if(count && length>0xffffffffUL/count)r->bad=1;else length*=count;}
 else r->bad=1;
 if(r->bad){free(s);return 0;}
 if(!grow((void**)&m->ext,&m->ce,m->ne+1,sizeof(External)) ||
    !grow((void**)&m->com,&m->cc,m->nc+1,sizeof(Communal))){free(s);return 0;}
 ext=&m->ext[m->ne++];ext->name=dup_(s);ext->type=0;ext->local=local;
 if(!ext->name){free(s);return 0;}
 co=&m->com[m->nc++];co->name=s;co->size=length;co->extidx=m->ne;co->local=local;
 }}
 else if(t==0x90||t==0x91||t==0xb6||t==0xb7){a=ix(r);b=ix(r);if(!b)(void)num(r,2);while(r->i<r->n&&!r->bad){s=string_(r);off=num(r,(t&1)?4:2);(void)ix(r);if(!s)return 0;if(!grow((void**)&m->pub,&m->cp,m->np+1,sizeof(Public))){free(s);return 0;}pub=&m->pub[m->np++];pub->name=s;pub->seg=b;pub->grp=a;pub->offset=off;pub->local=(t==0xb6||t==0xb7);}}
 else if(t==0xa0||t==0xa1){a=ix(r);off=num(r,(t&1)?4:2);if(r->bad)return 0;if(!grow((void**)&m->data,&m->cd,m->nd+1,sizeof(Data)))return 0;d=&m->data[m->nd++];memset(d,0,sizeof(*d));d->seg=a;d->offset=off;d->length=r->n-r->i;d->bytes=(U8*)malloc(d->length?d->length:1);if(!d->bytes)return 0;memcpy(d->bytes,r->p+r->i,d->length);r->i=r->n;m->last_data=m->nd;}
 else if(t==0xa2||t==0xa3){U8 *buf=NULL;size_t used=0,cap=0,raw_start;
 a=ix(r);off=num(r,(t&1)?4:2);
 if(r->bad)return 0;
 raw_start=r->i;
 while(r->i<r->n){if(!iter_block(r,(t&1)?4:2,0,&buf,&used,&cap)){free(buf);return 0;}}
 if(!grow((void**)&m->data,&m->cd,m->nd+1,sizeof(Data))){free(buf);return 0;}
 d=&m->data[m->nd++];memset(d,0,sizeof(*d));d->seg=a;d->offset=off;d->length=used;d->bytes=buf;
 d->encoded_len=r->n-raw_start;d->countsize=(t&1)?4:2;
 d->encoded=(U8*)malloc(d->encoded_len?d->encoded_len:1);
 if(!d->encoded)return 0;
 memcpy(d->encoded,r->p+raw_start,d->encoded_len);
 m->last_data=m->nd;
}
 else if(t==0x9c||t==0x9d){while(r->i<r->n&&!r->bad){unsigned x=rd(r),isframe,thread,method,index,fixdat,fram,tar,dispflag,location,at;Fixup *f;
 if(!(x&0x80)){isframe=(x>>6)&1;thread=x&3;method=(x>>2)&7;index=0;if(method<4 || (isframe&&method==4))index=ix(r);if(isframe){m->frame[thread].method=method;m->frame[thread].idx=index;}else{m->target[thread].method=method;m->target[thread].idx=index;}continue;}
 at=((x&3)<<8)|rd(r);location=(x>>2)&15;fixdat=rd(r);fram=(fixdat>>4)&7;tar=fixdat&3;dispflag=(fixdat&4)==0;
 if(!grow((void**)&m->fix,&m->cf,m->nf+1,sizeof(Fixup)))return 0;
f=&m->fix[m->nf++];memset(f,0,sizeof(*f));f->data_id=m->last_data;f->at=at;f->loc=location;f->mode=(x>>6)&1;f->has_disp=dispflag;
 if(fixdat&0x80){f->frame_method=m->frame[fram&3].method;f->frame_idx=m->frame[fram&3].idx;}else{f->frame_method=fram;if(fram<=3)f->frame_idx=ix(r);}
 if(fixdat&0x08){f->target_method=m->target[tar].method;f->target_idx=m->target[tar].idx;}else{f->target_method=tar;f->target_idx=ix(r);}
 if(dispflag)f->disp=num(r,(t&1)?4:2);
 if(f->data_id){d=&m->data[f->data_id-1];f->seg=d->seg;}else return 0;
 }}
 else if(t==0x88){a=rd(r);b=rd(r);if(b==0xa0&&r->i<r->n){c=rd(r);if(c==1){Import *imp;unsigned ordinalflag=rd(r);if(!grow((void**)&m->imports,&m->ci,m->ni+1,sizeof(Import)))return 0;imp=&m->imports[m->ni++];memset(imp,0,sizeof(*imp));imp->byordinal=ordinalflag!=0;imp->internal=string_(r);imp->dll=string_(r);if(imp->byordinal)imp->ordinal=(unsigned)num(r,2);else imp->symbol=string_(r);if(!imp->internal||!imp->dll||(!imp->byordinal&&!imp->symbol))return 0;}}(void)a;}
 else r->i=r->n;
 return !r->bad;
}
static int put_member(Members *ms,const char *file,unsigned long offset,int lib,Module *m){Member *x;if(!grow((void**)&ms->v,&ms->cap,ms->n+1,sizeof(Member)))return 0;x=&ms->v[ms->n++];memset(x,0,sizeof(*x));x->file=dup_(file);if(!x->file)return 0;x->offset=offset;x->m=*m;x->islib=lib;x->selected=!lib;memset(m,0,sizeof(*m));return 1;}
static int input_file(Members *ms,const char *file){FILE *fp;long len;U8 *buf;size_t size,pos,page=0,limit,base=0;int lib,inside=0,ok=1;Module m;memset(&m,0,sizeof(m));fp=fopen(file,"rb");if(!fp){perror(file);return 0;}if(fseek(fp,0,SEEK_END)|| (len=ftell(fp))<0 || fseek(fp,0,SEEK_SET)){fclose(fp);return 0;}if((unsigned long)len>64UL*1024UL*1024UL){fclose(fp);fprintf(stderr,"%s: exceeds 64 MiB limit\n",file);return 0;}size=(size_t)len;buf=(U8*)malloc(size?size:1);if(!buf){fclose(fp);return 0;}if(fread(buf,1,size,fp)!=size){free(buf);fclose(fp);return 0;}fclose(fp);lib=size>3&&buf[0]==0xf0;pos=0;limit=size;
 if(lib){page=(size_t)buf[1]+((size_t)buf[2]<<8)+3; if(page<16||page>size){ok=0;goto done;}pos=page;if(size>=7){U32 dict=(U32)buf[3]|((U32)buf[4]<<8)|((U32)buf[5]<<16)|((U32)buf[6]<<24);if(dict && dict<size)limit=(size_t)dict;}}
 while(pos<limit){unsigned t,n,sum=0;size_t end,j;Reader r;if(limit-pos<3){ok=0;break;}t=buf[pos];n=(unsigned)buf[pos+1]|((unsigned)buf[pos+2]<<8);end=pos+3+(size_t)n;if(!n||end>limit){ok=0;break;}for(j=pos;j<end;j++)sum=(sum+buf[j])&255;if(sum&&buf[end-1]!=0){ok=0;fprintf(stderr,"%s: checksum error at %lu\n",file,(unsigned long)pos);break;}
 if(t==0x80||t==0x82){if(inside){ok=0;break;}inside=1;base=pos;}
 r.p=buf+pos+3;r.n=n-1;r.i=0;r.bad=0;if(!parse_record(&m,t,&r)){ok=0;fprintf(stderr,"%s: bad record %02X at %lu\n",file,t,(unsigned long)pos);break;}
 pos=end;
 if(t==0x8a||t==0x8b){if(!put_member(ms,file,(unsigned long)base,lib,&m)){ok=0;break;}inside=0;if(lib&&pos%page)pos+=page-pos%page;}
 if(t==0xf1)break;
 }
 if(inside)ok=0;
 done:module_free(&m);free(buf);if(!ok)fprintf(stderr,"%s: malformed or unsupported input\n",file);return ok;}
static int exports(const Module *m,const char *symbol){unsigned j;for(j=0;j<m->np;j++)if(!strcmp(m->pub[j].name,symbol))return 1;for(j=0;j<m->ni;j++)if(!strcmp(m->imports[j].internal,symbol))return 1;for(j=0;j<m->nc;j++)if(!m->com[j].local&&!strcmp(m->com[j].name,symbol))return 1;return 0;}
static int linker_boundary(const char *symbol){return !strcmp(symbol,"_edata")||!strcmp(symbol,"_end");}
static int selected_export(const Members *ms,const char *symbol){unsigned i;if(linker_boundary(symbol))return 1;for(i=0;i<ms->n;i++)if(ms->v[i].selected&&exports(&ms->v[i].m,symbol))return 1;return 0;}
static int select_members(Members *ms){unsigned changes=0,i,j,k,rank=0;for(i=0;i<ms->n;i++)if(ms->v[i].selected)ms->v[i].selection_rank=rank++;do{changes=0;for(i=0;i<ms->n;i++)if(ms->v[i].selected){Module *m=&ms->v[i].m;for(j=0;j<m->ne;j++){const char *s=m->ext[j].name;if(selected_export(ms,s))continue;for(k=0;k<ms->n;k++)if(ms->v[k].islib&&!ms->v[k].selected&&exports(&ms->v[k].m,s)){ms->v[k].selected=1;ms->v[k].selection_rank=rank++;changes++;break;}}}}while(changes);return 1;}
/* Boundary symbol accounting: these are linker-synthesized globals, not
 * OMF object exports. At this stage they are symbolic, because the final
 * initialized/uninitialized object ordering and alignment are not fixed.
 * Resolve them for archive selection, but never publish invented VAs. */
static void print_boundaries(const Members *ms){unsigned i,j,n=0;int a=0,b=0;
 for(i=0;i<ms->n;i++)if(ms->v[i].selected){const Module *m=&ms->v[i].m;
 for(j=0;j<m->ne;j++){if(!strcmp(m->ext[j].name,"_edata")){a=1;n++;}
 else if(!strcmp(m->ext[j].name,"_end")){b=1;n++;}}
 }
 puts("LINKER BOUNDARIES (synthetic symbol resolution; VA pending final layout)");
 if(a)puts("  _edata RESOLVED synthetic initialized-data boundary VA=DEFERRED");
 if(b)puts("  _end   RESOLVED synthetic uninitialized-data boundary VA=DEFERRED");
 printf("BOUNDARY SUMMARY distinct=%u references=%u addressable=0\n",(unsigned)(a+b),n);
}
/* A COMDEF is a tentative definition: merge duplicate global declarations by
 * maximum size, and let an ordinary PUBDEF/import supersede the allocation.
 * This is a *planning* BSS offset, not yet a final LE object address. */
static int strong_export(const Members *ms,const char *sym){unsigned i,j;
 for(i=0;i<ms->n;i++)if(ms->v[i].selected){const Module *m=&ms->v[i].m;
 for(j=0;j<m->np;j++)if(!strcmp(m->pub[j].name,sym))return 1;
 for(j=0;j<m->ni;j++)if(!strcmp(m->imports[j].internal,sym))return 1;
 }
 return 0;}
static void print_common_plan(const Members *ms){unsigned i,j,k,unique=0;U32 offset=0;
 puts("COMMON PLAN (tentative global BSS, DWORD alignment; not an LE layout)");
 for(i=0;i<ms->n;i++)if(ms->v[i].selected){const Module *m=&ms->v[i].m;
 for(j=0;j<m->nc;j++){const Communal *c=&m->com[j];U32 maximum=c->size;
 if(c->local){printf("  LOCAL %s module=%s size=%lu (not globally exported)\n",c->name,m->name?m->name:"?",(unsigned long)c->size);continue;}
 for(k=0;k<i;k++)if(ms->v[k].selected){const Module *previous=&ms->v[k].m;unsigned q;int found=0;
 for(q=0;q<previous->nc;q++)if(!previous->com[q].local&&!strcmp(previous->com[q].name,c->name)){found=1;break;}
 if(found)break;
 }
 if(k<i)continue;
 for(k=i;k<ms->n;k++)if(ms->v[k].selected){const Module *next=&ms->v[k].m;unsigned q;
 for(q=(k==i?j+1:0);q<next->nc;q++)if(!next->com[q].local&&!strcmp(next->com[q].name,c->name)&&next->com[q].size>maximum)maximum=next->com[q].size;
 }
 if(strong_export(ms,c->name)){printf("  OVERRIDE %s by strong symbol\n",c->name);continue;}
 if(offset>0xffffffffUL-3UL || maximum>0xffffffffUL-((offset+3UL)&~3UL)){fprintf(stderr,"common storage overflow\n");return;}
 offset=(offset+3UL)&~3UL;
 printf("  COMMON %-30s offset=%08lX size=%lu\n",c->name,(unsigned long)offset,(unsigned long)maximum);
 offset+=maximum;unique++;
 }}
 printf("COMMON SUMMARY globals=%u bss_bytes=%lu\n",unique,(unsigned long)offset);
}
static void print_plan(const Members *ms){unsigned i,j,mods=0,segs=0,pubs=0,exts=0,fixups=0,unresolved=0,comm=0;U32 base=0;
 puts("LINK386 R2 LINK PLAN (no executable emitted)");
 for(i=0;i<ms->n;i++)if(ms->v[i].selected){const Module *m=&ms->v[i].m;mods++;printf("MODULE %s [%s @%lu]\n",m->name?m->name:"?",ms->v[i].file,ms->v[i].offset);for(j=0;j<m->ns;j++){const Segment *s=&m->seg[j];U32 align=1;if(s->align==1)align=1;else if(s->align==2)align=2;else if(s->align==3)align=16;else if(s->align==4)align=256;else if(s->align==5)align=4;else if(s->align==6)align=4096;if(align>1)base=(base+align-1)&~(align-1);printf("  SEG %u %-16s size=%lu align=%lu layout=%08lX use32=%u group=%u\n",j+1,s->name,(unsigned long)s->length,(unsigned long)align,(unsigned long)base,s->use32,s->group);base+=s->length;segs++;}
 for(j=0;j<m->ng;j++){unsigned k;printf("  GROUP %u %s members=",j+1,m->grp[j].name);for(k=0;k<m->grp[j].n;k++)printf("%s%u",k?",":"",m->grp[j].segments[k]);puts("");}
 for(j=0;j<m->nc;j++){const Communal *c=&m->com[j];printf("  COMDEF %s size=%lu %s ext-index=%u\n",c->name,(unsigned long)c->size,c->local?"local":"global",c->extidx);comm++;}
 for(j=0;j<m->ni;j++){const Import *imp=&m->imports[j];printf("  IMPORT %s from %s ",imp->internal,imp->dll);if(imp->byordinal)printf("ordinal=%u\n",imp->ordinal);else printf("name=%s\n",imp->symbol);}
 for(j=0;j<m->np;j++){printf("  PUB %s seg=%u offset=%08lX\n",m->pub[j].name,m->pub[j].seg,(unsigned long)m->pub[j].offset);pubs++;}
 for(j=0;j<m->ne;j++){int found=selected_export(ms,m->ext[j].name);printf("  EXT %s %s\n",m->ext[j].name,found?"resolved":"UNRESOLVED");exts++;if(!found)unresolved++;}
 for(j=0;j<m->nf;j++){const Fixup *f=&m->fix[j];printf("  FIXUP seg=%u at=%08lX loc=%u %s frame=%u:%u target=%u:%u disp=%08lX\n",f->seg,f->data_id?(unsigned long)(m->data[f->data_id-1].offset+f->at):0,f->loc,f->mode?"segment-relative":"self-relative",f->frame_method,f->frame_idx,f->target_method,f->target_idx,(unsigned long)f->disp);fixups++;}
 }
 print_common_plan(ms);
 print_boundaries(ms);
 printf("SUMMARY selected_modules=%u segments=%u publics=%u externals=%u fixups=%u unresolved=%u commons=%u\n",mods,segs,pubs,exts,fixups,unresolved,comm);
}
int main(int argc,char **argv){Members ms;int i,ok=1,files=0;unsigned j;memset(&ms,0,sizeof(ms));if(argc<2){fprintf(stderr,"usage: link386r2 [-v] file.obj [file.lib ...]\n");return 2;}for(i=1;i<argc;i++){if(!strcmp(argv[i],"-v"))continue;files++;if(!input_file(&ms,argv[i]))ok=0;}if(ok&&files){select_members(&ms);print_plan(&ms);}for(j=0;j<ms.n;j++){free(ms.v[j].file);module_free(&ms.v[j].m);}free(ms.v);return !files?2:ok?0:1;}
