/* LINK386 R1: bounded Microsoft OMF object/library inventory. ISO C89. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef unsigned char U8;
typedef unsigned long U32;
typedef struct { const U8 *p; size_t n, at; int err; } Cur;
typedef struct { unsigned long records, modules, checksum_errors, malformed;
 unsigned long externals, publics, imports, segments, fixups, data_records;
 unsigned long bytes; } Stats;
typedef struct { char **v; size_t n, cap; } Strings;
static int verbose = 0;
static int quiet = 0;
static const char *rtype(unsigned x) {
 switch(x) {
 case 0x80:return "THEADR";case 0x82:return "LHEADR";case 0x88:return "COMENT";
 case 0x8a:return "MODEND";case 0x8b:return "MODEND32";
 case 0x8c:return "EXTDEF";case 0x8e:return "TYPDEF";
 case 0x90:return "PUBDEF";case 0x91:return "PUBDEF32";
 case 0x96:return "LNAMES";case 0x98:return "SEGDEF";case 0x99:return "SEGDEF32";
 case 0x9a:return "GRPDEF";case 0x9b:return "GRPDEF32";
 case 0x9c:return "FIXUPP";case 0x9d:return "FIXUPP32";
 case 0xa0:return "LEDATA";case 0xa1:return "LEDATA32";
 case 0xa2:return "LIDATA";case 0xa3:return "LIDATA32";
 case 0xb0:return "COMDEF";case 0xb2:return "BAKPAT";
 case 0xb4:return "LEXTDEF";case 0xb6:return "LPUBDEF";
 case 0xbc:return "CEXTDEF";case 0xc2:return "COMDAT";
 case 0xf0:return "LIBHDR";case 0xf1:return "LIBEND";
 default:return "UNKNOWN"; }
}
static unsigned byte(Cur *c) { if(c->at>=c->n){c->err=1;return 0;} return c->p[c->at++]; }
static unsigned index_(Cur *c) { unsigned a=byte(c); if(a&128) a=((a&127)<<8)|byte(c); return a; }
static U32 number(Cur *c, unsigned size) { U32 v=0;unsigned i;for(i=0;i<size;i++)v|=((U32)byte(c))<<(i*8);return v; }
static char *str(Cur *c) { unsigned len=byte(c);char *s;unsigned i;if(c->err||len>c->n-c->at){c->err=1;return NULL;} s=(char*)malloc(len+1);if(!s){c->err=1;return NULL;}for(i=0;i<len;i++)s[i]=(char)byte(c);s[len]=0;return s; }
static void clear(Strings *s){size_t i;for(i=0;i<s->n;i++)free(s->v[i]);free(s->v);memset(s,0,sizeof(*s));}
static int add(Strings *s,char *p){char **v;size_t cap;if(!p)return 0;if(s->n==s->cap){cap=s->cap?s->cap*2:16;v=(char**)realloc(s->v,cap*sizeof(char*));if(!v){free(p);return 0;}s->v=v;s->cap=cap;}s->v[s->n++]=p;return 1;}
static const char *name(const Strings *s,unsigned ix){return ix && ix<=s->n ?s->v[ix-1]:"?";}
static void printname(const char *label,const char *s){if(!quiet)printf("    %-10s %s\n",label,s?s:"<invalid>");}
static int parse(const U8 *data,size_t size,int lib, Stats *st,const char *filename){
 size_t pos=0, limit=size, page=0, dictpos=0;
 int active=0; Strings names;
 memset(&names,0,sizeof(names));
 if(lib){unsigned len;unsigned long blocks; if(size<16||data[0]!=0xf0){fprintf(stderr,"%s: invalid LIBHDR\n",filename);return 0;}
 len=(unsigned)data[1]|((unsigned)data[2]<<8);page=(size_t)len+3;
 if(len<7||page>size||page==0){fprintf(stderr,"%s: invalid library page size\n",filename);return 0;}
 dictpos=(size_t)data[3]|((size_t)data[4]<<8)|((size_t)data[5]<<16)|((size_t)data[6]<<24);
 blocks=(unsigned long)data[7]|((unsigned long)data[8]<<8);
 if(dictpos && dictpos<size)limit=dictpos;
 if(!quiet)printf("LIBRARY %s size=%lu page=%lu dictionary=%lu blocks=%lu\n",filename,(unsigned long)size,(unsigned long)page,(unsigned long)dictpos,blocks);
 pos=page;st->records++;
 } else if(!quiet)printf("OBJECT %s size=%lu\n",filename,(unsigned long)size);
 while(pos<limit){unsigned type,len,sum=0;size_t k,start,next;Cur c;int end;
 if(limit-pos<3){fprintf(stderr,"%s: truncated record header at %lu\n",filename,(unsigned long)pos);st->malformed++;break;}
 start=pos;type=data[pos];len=(unsigned)data[pos+1]|((unsigned)data[pos+2]<<8);next=pos+3+(size_t)len;
 if(len==0||next>limit){fprintf(stderr,"%s: malformed record %02X at %lu length=%u\n",filename,type,(unsigned long)pos,len);st->malformed++;break;}
 for(k=pos;k<next;k++)sum=(sum+data[k])&255;
 if(sum && data[next-1]!=0){st->checksum_errors++;fprintf(stderr,"%s: checksum mismatch at %lu type=%02X sum=%02X\n",filename,(unsigned long)pos,type,sum);}
 st->records++;
 if(verbose&&!quiet)printf("  @%08lX %-10s (%02X) length=%u checksum=%s\n",(unsigned long)pos,rtype(type),type,len,(sum && data[next-1]!=0)?"BAD":(sum?"OMITTED":"OK"));
 c.p=data+pos+3;c.n=len-1;c.at=0;c.err=0;
 if(type==0x80||type==0x82){char *s=str(&c);if(active)clear(&names);active=1;st->modules++;if(!quiet && (!lib || verbose))printf("  MODULE %lu @%lu %s\n",st->modules,(unsigned long)start,s?s:"<invalid>");free(s);}
 else if(type==0x96){while(!c.err&&c.at<c.n){if(!add(&names,str(&c))){c.err=1;break;}}if(verbose&&!quiet){size_t j;for(j=0;j<names.n;j++)printf("    LNAME[%lu] %s\n",(unsigned long)(j+1),names.v[j]);}}
 else if(type==0x8c||type==0xb4){while(!c.err&&c.at<c.n){char *s=str(&c);(void)index_(&c);st->externals++;if(verbose)printname("EXTDEF",s);free(s);}}
 else if(type==0x90||type==0x91||type==0xb6||type==0xb7){unsigned grp=index_(&c),seg=index_(&c);unsigned offsize=(type&1)?4:2;if(seg==0)(void)number(&c,2);while(!c.err&&c.at<c.n){char *s=str(&c);U32 off=number(&c,offsize);(void)index_(&c);st->publics++;if(verbose&&!quiet)printf("    PUBDEF     %s offset=%08lX grp=%u seg=%u\n",s?s:"<invalid>",(unsigned long)off,grp,seg);free(s);}}
 else if(type==0x98||type==0x99){unsigned ac=byte(&c),al=ac>>5;U32 seglen;if(al==0)(void)number(&c,2);seglen=number(&c,(type&1)?4:2);{unsigned sn=index_(&c),cl=index_(&c),ov=index_(&c);st->segments++;if(verbose&&!quiet)printf("    SEGDEF     %s class=%s overlay=%s size=%lu attr=%02X\n",name(&names,sn),name(&names,cl),name(&names,ov),(unsigned long)seglen,ac);}}
 else if(type==0x88){unsigned attr=byte(&c),cls=byte(&c);if(cls==0xa0&&c.at<c.n){unsigned sub=byte(&c);if(sub==1&&c.at<c.n){unsigned ord=byte(&c);char *internal=str(&c),*dll=str(&c);st->imports++;if(verbose&&!quiet)printf("    IMPORT     %s from %s %s",internal?internal:"?",dll?dll:"?",ord?"by ordinal":"by name");if(ord){U32 v=number(&c,2);if(!quiet&&verbose)printf(" %lu",(unsigned long)v);}else if(c.at<c.n){char *symbol=str(&c);if(!quiet&&verbose)printf(" (%s)",symbol?symbol:"?");free(symbol);}if(verbose&&!quiet)printf("\n");free(internal);free(dll);}}(void)attr;}
 else if(type==0x9c||type==0x9d){st->fixups++;}
 else if(type==0xa0||type==0xa1||type==0xa2||type==0xa3){st->data_records++;}
 if(c.err){st->malformed++;fprintf(stderr,"%s: malformed payload type=%02X offset=%lu\n",filename,type,(unsigned long)start);}
 end=(type==0x8a||type==0x8b);
 pos=next;
 if(end){active=0;clear(&names);if(lib){size_t rem=pos%page;if(rem)pos+=page-rem;}}
 if(type==0xf1)break;
 }
 if(active)clear(&names);
 if(!quiet)printf("SUMMARY records=%lu modules=%lu segments=%lu externals=%lu publics=%lu imports=%lu fixups=%lu data=%lu checksum_errors=%lu malformed=%lu\n",st->records,st->modules,st->segments,st->externals,st->publics,st->imports,st->fixups,st->data_records,st->checksum_errors,st->malformed);
 return st->checksum_errors==0&&st->malformed==0;
}
static int process(const char *filename){FILE *fp;long l;size_t n;U8 *b;int lib,ok;Stats st;memset(&st,0,sizeof(st));fp=fopen(filename,"rb");if(!fp){perror(filename);return 0;}if(fseek(fp,0,SEEK_END)!=0||(l=ftell(fp))<0||fseek(fp,0,SEEK_SET)!=0){fprintf(stderr,"%s: cannot seek\n",filename);fclose(fp);return 0;}if((unsigned long)l>64UL*1024UL*1024UL){fprintf(stderr,"%s: R1 safety cap 64 MiB\n",filename);fclose(fp);return 0;}b=(U8*)malloc((size_t)l+1);if(!b){fclose(fp);return 0;}n=fread(b,1,(size_t)l,fp);fclose(fp);if(n!=(size_t)l){free(b);fprintf(stderr,"%s: read failed\n",filename);return 0;}lib=(n>=3&&b[0]==0xf0);ok=parse(b,n,lib,&st,filename);free(b);return ok;}
int main(int argc,char **argv){int i,found=0,ok=1;if(argc<2){fprintf(stderr,"LINK386 R1 OMF inspector\nusage: link386r1 [-v] [-q] file.obj [file.lib ...]\n  -v show records/symbols/imports; -q validate silently\n");return 2;}for(i=1;i<argc;i++){if(!strcmp(argv[i],"-v")){verbose=1;continue;}if(!strcmp(argv[i],"-q")){quiet=1;continue;}found++;if(!process(argv[i]))ok=0;}return !found?2:ok?0:1;}
