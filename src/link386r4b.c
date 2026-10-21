/* LINK386 R4B: deliberately reference-assisted, bounded C/386 LE relinker.
 * ISO C89. Not a general-purpose native OMF-to-LE linker.
 * The original LINK386 LE supplies the already-linked runtime, loader fixups,
 * and LE tables. This program validates the reference and re-links one
 * application OMF _TEXT and _DATA using its own decoded FIXUPP32 records.
 */
#define main r2_internal_main
#include "link386r2.c"
#undef main
#include <errno.h>
#define LE_MAX (32UL*1024UL*1024UL)
#define MAX_LE_PAGES 256
#define MAX_LE_OBJECTS 16
#define MAX_WRITE_SITES 32

typedef struct {U32 base,size,first,pages,flags;} R4Object;
typedef struct {U8 *image;size_t len;U32 head,dataoff,pagesize,lastpagesize;unsigned np,no;R4Object objects[MAX_LE_OBJECTS];unsigned pagefile[MAX_LE_PAGES];} R4Le;
static int allow_data_edit=0;
static U32 crc32_(const U8*p,size_t n){U32 crc=0xffffffffUL;size_t i;unsigned j;for(i=0;i<n;i++){crc^=p[i];for(j=0;j<8;j++)crc=(crc&1)?(crc>>1)^0xedb88320UL:crc>>1;}return (crc^0xffffffffUL)&0xffffffffUL;}
static U32 u16le(const U8*p){return (U32)p[0]+((U32)p[1]<<8);}
static U32 u32le(const U8*p){return u16le(p)+(u16le(p+2)<<16);}
static void put32(U8*p,U32 v){p[0]=(U8)v;p[1]=(U8)(v>>8);p[2]=(U8)(v>>16);p[3]=(U8)(v>>24);}
static int range_(size_t n,U32 pos,U32 size){return pos<=n && size<=n-pos;}
static int error_(const char*txt){fprintf(stderr,"R4B ERROR: %s\n",txt);return 0;}
static int load_file_(const char*name,U8**bytes,size_t*len){FILE*fp;long n;U8*p;fp=fopen(name,"rb");if(!fp){perror(name);return 0;}
 if(fseek(fp,0,SEEK_END)||(n=ftell(fp))<0||fseek(fp,0,SEEK_SET)|| (unsigned long)n>LE_MAX){fclose(fp);return error_("input file exceeds safety limit");}
 p=(U8*)malloc(n?(size_t)n:1);if(!p){fclose(fp);return error_("allocation failure");}
 if(fread(p,1,(size_t)n,fp)!=(size_t)n){free(p);fclose(fp);return error_("input read failed");}
 fclose(fp);*bytes=p;*len=(size_t)n;return 1;}
static int le_decode_(R4Le*l,const char*name){unsigned i,j;U32 objtab,pagetab,fixpagetab,fixrecord,fixsize,dat;
 if(!load_file_(name,&l->image,&l->len))return 0;
 if(l->len!=11806||crc32_(l->image,l->len)!=0x41ae0ed4UL)return error_("reference scaffold fingerprint is not the verified Microsoft hello");
 if(!range_(l->len,0,0x40)||l->image[0]!='M'||l->image[1]!='Z')return error_("reference lacks MZ header");
 l->head=u32le(l->image+0x3c);if(!range_(l->len,l->head,0xb0)||memcmp(l->image+l->head,"LE",2))return error_("reference lacks LE header");
 l->pagesize=u32le(l->image+l->head+0x28);l->np=(unsigned)u32le(l->image+l->head+0x14);l->no=(unsigned)u32le(l->image+l->head+0x44);
 if(l->pagesize!=4096||!l->np||l->np>MAX_LE_PAGES||!l->no||l->no>MAX_LE_OBJECTS)return error_("unsupported reference LE page/object layout");
 objtab=u32le(l->image+l->head+0x40)+l->head;
 pagetab=u32le(l->image+l->head+0x48)+l->head;
 fixpagetab=u32le(l->image+l->head+0x68)+l->head;
 fixrecord=u32le(l->image+l->head+0x6c)+l->head;
 dat=u32le(l->image+l->head+0x80);l->dataoff=dat;
 l->lastpagesize=u32le(l->image+l->head+0x2c);
 if(!l->lastpagesize||l->lastpagesize>l->pagesize)return error_("invalid final-page byte count");
 if(!range_(l->len,objtab,(U32)(l->no*24))||!range_(l->len,pagetab,(U32)l->np*4)||!range_(l->len,fixpagetab,(U32)(l->np+1)*4)||!range_(l->len,dat,((U32)l->np-1)*l->pagesize+l->lastpagesize))return error_("LE tables or data out of bounds");
 for(i=0;i<l->no;i++){const U8*p=l->image+objtab+i*24;R4Object*o=&l->objects[i];o->size=u32le(p);o->base=u32le(p+4);o->flags=u32le(p+8);o->first=u32le(p+12);o->pages=u32le(p+16);if(!o->first||o->first>l->np||o->pages>l->np-o->first+1)return error_("LE object page range invalid");}
 for(i=0;i<l->np;i++){const U8*p=l->image+pagetab+4*i;unsigned pn=((unsigned)p[0]<<16)|((unsigned)p[1]<<8)|(unsigned)p[2];if(!pn||pn>l->np||p[3])return error_("unsupported page mapping");for(j=0;j<i;j++)if(l->pagefile[j]==pn)return error_("duplicate LE physical page");l->pagefile[i]=pn;}
 fixsize=u32le(l->image+fixpagetab+4*l->np);
 if(!range_(l->len,fixrecord,fixsize))return error_("LE fixup stream not in bounds");
 for(i=0;i<l->np;i++){U32 a=u32le(l->image+fixpagetab+4*i),b=u32le(l->image+fixpagetab+4*(i+1));if(a>b||b>fixsize)return error_("LE fixup page offsets invalid");}
 return 1;}
static int va_ptr_(R4Le*l,unsigned obj,U32 offset,size_t n,U8**ptr){R4Object*o;unsigned pg,logical,physical;U32 off;
 if(!obj||obj>l->no)return 0;
 o=&l->objects[obj-1];if(offset>o->size||n>o->size-offset)return 0;
 pg=(unsigned)(offset/l->pagesize);off=offset%l->pagesize;if(n>l->pagesize-off||pg>=o->pages)return 0;
 logical=(unsigned)o->first-1+pg;if(logical>=l->np)return 0;physical=l->pagefile[logical];
 if(!range_(l->len,l->dataoff+((U32)physical-1)*l->pagesize+off,(U32)n))return 0;
 *ptr=l->image+l->dataoff+((U32)physical-1)*l->pagesize+off;return 1;}
static int scan_segment_(R4Le*l,unsigned obj,const Data*d,const U8*mask,U32*offset){R4Object*o=&l->objects[obj-1];U32 pos,i,count=0,found=0;U8*at;
 if(d->length>o->size)return error_("source contribution is larger than reference object");
 for(pos=0;pos<=o->size-d->length;pos++){if(!va_ptr_(l,obj,pos,d->length,&at))continue;
 for(i=0;i<d->length;i++)if(!mask[i]&&at[i]!=d->bytes[i])break;
 if(i==d->length){found=pos;count++;if(count>1)break;}}
 if(count!=1){fprintf(stderr,"R4B: contribution candidate matches=%lu (expected exactly one)\n",(unsigned long)count);return 0;}
 *offset=found;return 1;}
/* R3S observed LE loader-fixup forms: non-additive, targets internal objects
 * or DOSCALLS ordinal imports; source offsets can be grouped in a list. */
static int validate_loader_fixups_(R4Le*l,unsigned*internals,unsigned*imports){U32 table=l->head+u32le(l->image+l->head+0x68),fixrec=l->head+u32le(l->image+l->head+0x6c);unsigned pg;U32 p,end;
 *internals=*imports=0;
 for(pg=0;pg<l->np;pg++){p=fixrec+u32le(l->image+table+pg*4);end=fixrec+u32le(l->image+table+(pg+1)*4);
 while(p<end){unsigned source,fl,listed,n,method,target_index,width,k,offs[256];U32 target;U8*ptr;
 if(p+2>end)return error_("truncated LE fixup record");
 source=l->image[p++];fl=l->image[p++];listed=(source&0x20)!=0;
 if((source&15)!=7&&(source&15)!=8)return error_("unsupported loader source fixup type");
 if(fl&0x44)return error_("additive or long-module fixup not supported");
 if(listed){if(p>=end)return error_("truncated fixup count");n=l->image[p++];if(!n)return error_("empty fixup source list");}else{n=1;if(p+2>end)return error_("truncated fixup offset");offs[0]=(unsigned)(short)u16le(l->image+p);p+=2;}
 if(p>=end)return error_("truncated fixup target");
 method=fl&3;target_index=l->image[p++];
 if(method==0){width=(fl&0x10)?4:2;if(!target_index||target_index>l->no)return error_("bad internal fixup target");}
 else if(method==1){width=(fl&0x10)?4:((fl&0x80)?1:2);if(target_index!=1)return error_("unexpected import module");}
 else return error_("unsupported loader fixup target mode");
 if(p+width>end)return error_("truncated fixup target value");
 target=width==1?l->image[p]:width==2?u16le(l->image+p):u32le(l->image+p);p+=width;
 if(!target)return error_("zero fixup target");
 if(listed){if(p+2*n>end)return error_("truncated LE source list");for(k=0;k<n;k++){offs[k]=(unsigned)(short)u16le(l->image+p);p+=2;}}
 for(k=0;k<n;k++){unsigned oi,objoff=0,obji=0;int signedoff=(int)(short)offs[k];
 /* Fixup offsets are signed because overlapping fixups can begin before page. */
 for(oi=0;oi<l->no;oi++)if(pg>=l->objects[oi].first-1&&pg<l->objects[oi].first-1+l->objects[oi].pages){obji=oi+1;objoff=(unsigned)(pg-(l->objects[oi].first-1))*l->pagesize;break;}
 if(!obji||signedoff < -3 || signedoff>(int)l->pagesize-1)return error_("source fixup offset outside mapped page");
 if(signedoff>=0 && !va_ptr_(l,obji,objoff+(unsigned)signedoff,1,&ptr)){
 /* Original LE may legally have relocations in last mapped page padding. */
 if(objoff+(unsigned)signedoff>=l->objects[obji-1].pages*l->pagesize)return error_("bad mapped source site");}
 if(method==0)(*internals)++;else(*imports)++;
 }
 }
 if(p!=end)return error_("LE fixup page parsing did not converge");}
 return 1;}
/* Only the application's one _TEXT and one initialized _DATA contribution
 * are mutable. Reference provides all linked runtime bytes and fixup records. */
static int relink_(Members*ms,R4Le*l){const Module*app=NULL;const Segment *code=NULL,*data=NULL;const Data *ct=NULL,*dt=NULL;unsigned i,j,modcount=0,fixes=0,imports=0,internals=0;U8 cmask[1024],dmask[1024];U32 codeoff,dataoff,codeva,datava,printfva=0;U8*ref,*page;U32 originalval[2];
 for(i=0;i<ms->n;i++)if(ms->v[i].selected){modcount++;if(!ms->v[i].islib){if(app)return error_("R4B supports exactly one application object");app=&ms->v[i].m;}}
 if(!app||modcount<2||app->ns<2||app->nd!=2||app->nf!=2)return error_("input not supported: requires two application data records and two fixups");
 if(l->no!=2||l->np!=3||l->objects[0].base!=0x10000UL||l->objects[1].base!=0x20000UL)return error_("reference is not the expected two-object hello layout");
 if(u32le(l->image+l->head+0x18)!=1||u32le(l->image+l->head+0x1c)!=0x20)return error_("unsupported LE entry point");
 for(i=0;i<app->ns;i++)if(!strcmp(app->seg[i].name,"_TEXT")){if(code)return error_("ambiguous code segment");code=&app->seg[i];}
 else if(!strcmp(app->seg[i].name,"_DATA")){if(data)return error_("ambiguous data segment");data=&app->seg[i];}
 if(!code||!data||!code->use32||!data->use32)return error_("application missing USE32 _TEXT/_DATA");
 for(i=0;i<app->nd;i++)if(app->data[i].seg==(unsigned)(code-app->seg+1))ct=&app->data[i];else if(app->data[i].seg==(unsigned)(data-app->seg+1))dt=&app->data[i];
 if(!ct||!dt||ct->offset||dt->offset||ct->length>sizeof(cmask)||dt->length>sizeof(dmask))return error_("unsupported LEDATA/LIDATA structure");
 memset(cmask,0,sizeof(cmask));memset(dmask,0,sizeof(dmask));
 for(i=0;i<app->nf;i++){const Fixup*f=&app->fix[i];if(f->data_id!=(unsigned)(ct-app->data+1)||f->seg!=(unsigned)(code-app->seg+1)||f->loc!=9||f->at>ct->length||ct->length-f->at<4)return error_("unsupported application fixup form");for(j=0;j<4;j++)cmask[f->at+j]=1;}
 if(!scan_segment_(l,1,ct,cmask,&codeoff))return error_("reference code signature mismatch");
 codeva=l->objects[0].base+codeoff;
 if(!va_ptr_(l,1,codeoff,ct->length,&ref))return error_("reference code unavailable");
 if(!allow_data_edit){if(!scan_segment_(l,2,dt,dmask,&dataoff))return error_("reference data signature mismatch");}
 else {U32 pointer=u32le(ref+1);if(pointer<l->objects[1].base)return error_("source data pointer not in data object");dataoff=pointer-l->objects[1].base;
 if(!va_ptr_(l,2,dataoff,dt->length,&page))return error_("source data pointer out of range");}
 datava=l->objects[1].base+dataoff;
 /* Require the exact assembly shape and source target identity; do not guess. */
 if(ct->length!=16||dt->length!=5||ct->bytes[0]!=0x68||ct->bytes[5]!=0xe8||memcmp(ct->bytes+10,"\x83\xc4\x04\xc3\x90\x90",6))return error_("not the audited C/386 hello code shape");
 if(codeva!=0x10010UL||datava!=0x20044UL)return error_("layout is not independently verified hello position");
 if(!va_ptr_(l,1,codeoff,16,&ref))return error_("reference code span inaccessible");
 originalval[0]=u32le(ref+1);originalval[1]=u32le(ref+6);
 printfva=codeva+10+(U32)originalval[1];
 if(printfva<l->objects[0].base||printfva>=l->objects[0].base+l->objects[0].size)return error_("reference _printf target invalid");
 if(!selected_export(ms,"_printf"))return error_("LIBC.LIB did not resolve _printf");
 if(!validate_loader_fixups_(l,&internals,&imports))return 0;
 if(internals!=159||imports!=12)return error_("reference relocation oracle mismatch");
 /* Stamp real source contributions, not the original linked bytes. */
 memcpy(ref,ct->bytes,ct->length);if(!va_ptr_(l,2,dataoff,dt->length,&page))return error_("reference data span inaccessible");memcpy(page,dt->bytes,dt->length);
 for(i=0;i<app->nf;i++){const Fixup*f=&app->fix[i];U32 val,addend,target=0,at=codeva+f->at;
 if(!va_ptr_(l,1,codeoff+f->at,4,&page))return error_("fixup source outside code");
 addend=u32le(page);
 if(f->target_method==0&&f->target_idx==(unsigned)(data-app->seg+1)&&f->mode==1){target=datava;}
 else if(f->target_method==2&&f->target_idx&&f->target_idx<=app->ne&&!strcmp(app->ext[f->target_idx-1].name,"_printf")&&!f->mode){target=printfva;}
 else return error_("unsupported or unresolved application fixup target");
 val=target+(f->has_disp?f->disp:0)+addend;if(!f->mode)val=val-(at+4);put32(page,val);fixes++;
 printf("R4B APPLY FIXUP32 site=%08lX target=%08lX value=%08lX mode=%s\n",(unsigned long)at,(unsigned long)target,(unsigned long)(val&0xffffffffUL),f->mode?"absolute":"pc-relative");}
 printf("R4B VALIDATED selected=%u app_text=%08lX app_data=%08lX _printf=%08lX application_fixups=%u inherited_loader_sites=%u+%u\n",modcount,(unsigned long)codeva,(unsigned long)datava,(unsigned long)printfva,fixes,internals,imports);
 if(u32le(ref+1)!=originalval[0]||u32le(ref+6)!=originalval[1])return error_("application relocations disagree with original LINK386");
 return 1;}
static int save_(const R4Le*l,const char*filename){FILE*fp=fopen(filename,"wb");int ok;if(!fp){perror(filename);return 0;}ok=fwrite(l->image,1,l->len,fp)==l->len;if(fclose(fp))ok=0;if(!ok){remove(filename);return error_("writing output failed");}printf("R4B EMITTED %s bytes=%lu mode=REFERENCE_ASSISTED\n",filename,(unsigned long)l->len);return 1;}
int main(int argc,char**argv){Members ms;R4Le l;const char*ref=NULL,*out=NULL;unsigned i,nfiles=0;int j,ok=1;memset(&ms,0,sizeof(ms));memset(&l,0,sizeof(l));
 for(j=1;j<argc;j++){if(!strcmp(argv[j],"--allow-data-edit"))allow_data_edit=1;else if(!strcmp(argv[j],"-r")&&j+1<argc)ref=argv[++j];else if(!strcmp(argv[j],"-o")&&j+1<argc)out=argv[++j];else if(argv[j][0]=='-'){ok=error_("unknown option");break;}else{if(!input_file(&ms,argv[j]))ok=0;nfiles++;}}
 if(!ref||!out||nfiles!=3){fputs("usage: link386r4b [--allow-data-edit] -r microsoft-hi.exe -o output.exe hi.obj LIBC.LIB OS2386.LIB\n",stderr);ok=0;}
 if(ok){select_members(&ms);ok=le_decode_(&l,ref)&&relink_(&ms,&l);if(ok)ok=save_(&l,out);}
 free(l.image);for(i=0;i<ms.n;i++){free(ms.v[i].file);module_free(&ms.v[i].m);}free(ms.v);return ok?0:1;}
