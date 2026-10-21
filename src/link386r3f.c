/* R3F deterministic, conservative LINK386 layout planner. No LE emission. */
#define main link386_r2_unused_main
#include "link386r2.c"
#undef main
#define MAX_CONTRIB 8192
#define MAX_COMMON 4096
#define R3F_MAX32 0xffffffffUL
typedef struct { unsigned member,seg,kind; U32 off,size,align; } Contribution;
typedef struct { const char *name; U32 size,off; unsigned member,local; } CommonPlace;
static Contribution contrib[MAX_CONTRIB];
static CommonPlace commons[MAX_COMMON];
static unsigned cn,kn;
static U32 ends[3];
static int same_(const char*a,const char*b){return !strcmp(a?a:"",b?b:"");}
static unsigned category(const Segment*s){const char *c=s->class_name;
 if(same_(c,"BSS")||same_(c,"STACK")||same_(s->name,"_BSS")||same_(s->name,"STACK"))return 2;
 if(same_(c,"CODE")||same_(c,"TEXT")||same_(s->name,"_TEXT"))return 0;
 return 1;
}
static U32 alignment(const Segment*s){switch(s->align){case 1:return 1;case 2:return 2;case 3:return 16;case 4:return 256;case 5:return 4;case 6:return 4096;default:return 1;}}
static int align_(U32 x,U32 a,U32*out){if(!a||x>R3F_MAX32-(a-1))return 0;*out=(x+a-1)&~(a-1);return 1;}
static int place_segments(const Members*ms){unsigned cat,i,j;U32 pos;
 cn=0;for(cat=0;cat<3;cat++){pos=0;
 for(i=0;i<ms->n;i++)if(ms->v[i].selected){const Module*m=&ms->v[i].m;
 for(j=0;j<m->ns;j++){const Segment*s=&m->seg[j];U32 a=alignment(s);
 if(category(s)!=cat)continue;
 if(cn==MAX_CONTRIB||!align_(pos,a,&pos)||s->length>R3F_MAX32-pos)return 0;
 contrib[cn].member=i;contrib[cn].seg=j+1;contrib[cn].kind=cat;contrib[cn].off=pos;contrib[cn].size=s->length;contrib[cn].align=a;cn++;pos+=s->length;
 }
 }
 ends[cat]=pos;
 }return 1;}
static int common_exists(const char *name,unsigned member,unsigned local){unsigned k;for(k=0;k<kn;k++)if(commons[k].local==local&&same_(commons[k].name,name)&&(!local||commons[k].member==member))return (int)k;return -1;}
static int place_commons(const Members*ms){unsigned i,j,k;U32 pos=ends[2];kn=0;
 for(i=0;i<ms->n;i++)if(ms->v[i].selected){const Module*m=&ms->v[i].m;
 for(j=0;j<m->nc;j++){const Communal*c=&m->com[j];int n;if(!c->local&&strong_export(ms,c->name))continue;
 n=common_exists(c->name,i,c->local);
 if(n>=0){if(c->size>commons[n].size)commons[n].size=c->size;continue;}
 if(kn==MAX_COMMON){return 0;}
 commons[kn].name=c->name;commons[kn].size=c->size;commons[kn].member=i;commons[kn].local=c->local;commons[kn].off=0;kn++;
 }}
 for(k=0;k<kn;k++){if(!align_(pos,4,&pos)||commons[k].size>R3F_MAX32-pos)return 0;commons[k].off=pos;pos+=commons[k].size;}ends[2]=pos;return 1;}
static void report(const Members*ms){unsigned cat,i,j,k,n=0;unsigned long checksum=0;
 puts("LINK386 R3F DETERMINISTIC CONTRIBUTION PLAN (NOT MICROSOFT-EXACT; NO EXECUTABLE)");
 for(cat=0;cat<3;cat++){printf("LAYOUT REGION %s\n",cat==0?"CODE":cat==1?"INITIALIZED":"BSS_STACK");
 for(k=0;k<cn;k++)if(contrib[k].kind==cat){const Contribution*c=&contrib[k];const Module*m=&ms->v[c->member].m;const Segment*s=&m->seg[c->seg-1];printf("  SEG module=%s name=%s class=%s combine=%u use32=%u offset=%08lX size=%lu align=%lu\n",m->name?m->name:"?",s->name,s->class_name,s->combine,s->use32,(unsigned long)c->off,(unsigned long)c->size,(unsigned long)c->align);n++;checksum+=c->size;}
 printf("REGION SUMMARY kind=%u contributions=%u size=%lu\n",cat,n,(unsigned long)ends[cat]);n=0;}
 puts("COMMON ADDRESSES (REGION-RELATIVE, after uninitialized contributions)");
 for(k=0;k<kn;k++)printf("  COMMON %s scope=%s offset=%08lX size=%lu\n",commons[k].name,commons[k].local?"local":"global",(unsigned long)commons[k].off,(unsigned long)commons[k].size);
 puts("PUBLIC ADDRESSES (REGION-RELATIVE)");
 for(k=0;k<cn;k++){const Contribution*c=&contrib[k];const Module*m=&ms->v[c->member].m;
 for(j=0;j<m->np;j++)if(m->pub[j].seg==c->seg){const Public*p=&m->pub[j];if(p->offset<=c->size)printf("  PUB %s region=%u offset=%08lX module=%s\n",p->name,c->kind,(unsigned long)(c->off+p->offset),m->name?m->name:"?");else printf("  PUB-INVALID %s segment-overflow\n",p->name);}}
 printf("BOUNDARY-CANDIDATE _edata initialized_region_end=%08lX _end bss_region_end=%08lX (NOT FINAL VAs)\n",(unsigned long)ends[1],(unsigned long)ends[2]);
 printf("R3F SUMMARY contributions=%u commons=%u code=%lu data=%lu uninitialized=%lu total_contribution_bytes=%lu\n",cn,kn,(unsigned long)ends[0],(unsigned long)ends[1],(unsigned long)ends[2],checksum);
 (void)i;
}
int main(int argc,char**argv){Members ms;int i,ok=1,files=0;unsigned j;memset(&ms,0,sizeof(ms));if(argc<2){fprintf(stderr,"usage: link386r3f file.obj ... file.lib ...\n");return 2;}
 for(i=1;i<argc;i++){files++;if(!input_file(&ms,argv[i]))ok=0;}
 if(ok&&files){select_members(&ms);if(!place_segments(&ms)||!place_commons(&ms)){fprintf(stderr,"R3F: contribution/layout overflow\n");ok=0;}else report(&ms);}
 for(j=0;j<ms.n;j++){free(ms.v[j].file);module_free(&ms.v[j].m);}free(ms.v);return !files?2:ok?0:1;}
