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
/* R4L: generic archive dependency-level ordering (no names or oracle offsets).
 * Module depth 0 = command-line objects; each selected archive member is
 * assigned the minimum number of EXTDEF edges from a root module. At each
 * depth the original archive member order breaks ties. */
static int r4l_order_mode=0;
#ifdef R4L_LINKER
static unsigned *r4l_depths=NULL;
static unsigned r4l_depth_count=0;
static int r4l_assign_depths(const Members*ms){
 unsigned i,j,k,level,progress,unseen=~0U;
 unsigned *d=(unsigned*)malloc(ms->n*sizeof(unsigned));
 if(!d)return 0;
 for(i=0;i<ms->n;i++)d[i]=ms->v[i].selected&&!ms->v[i].islib?0:unseen;
 for(level=0;level<ms->n;level++){
  progress=0;
  for(i=0;i<ms->n;i++)if(d[i]==level){
   const Module*m=&ms->v[i].m;
   for(j=0;j<m->ne;j++){
    const char*sym=m->ext[j].name;
    if(m->ext[j].local)continue;
    for(k=0;k<ms->n;k++)if(ms->v[k].selected && d[k]==unseen &&
         exports(&ms->v[k].m,sym)){
      d[k]=level+1;progress++;break;
    }
   }
  }
  if(!progress)break;
 }
 for(i=0;i<ms->n;i++)if(ms->v[i].selected&&d[i]==unseen){
   fprintf(stderr,"R4L: selected but unreachable member %s\n",ms->v[i].m.name);
   free(d);return 0;
 }
 free(r4l_depths);r4l_depths=d;r4l_depth_count=(unsigned)ms->n;
 return 1;
}

#endif /* R4L_LINKER */
static int place_segments(const Members*ms){unsigned cat,i,j;U32 pos;
 cn=0;for(cat=0;cat<3;cat++){pos=0;
#ifdef R4L_LINKER
 if(r4l_order_mode&&(cat==0||cat==1)){
  typedef struct {unsigned member,seg;unsigned depth;} R4LPick;
  R4LPick picks[MAX_CONTRIB],save;unsigned np=0,n,z;
  if(!r4l_depths||r4l_depth_count!=ms->n)return 0;
  for(i=0;i<ms->n;i++)if(ms->v[i].selected){
   const Module*m=&ms->v[i].m;
   for(j=0;j<m->ns;j++)if(category(&m->seg[j])==cat){
    if(np>=MAX_CONTRIB)return 0;
    picks[np].member=i;picks[np].seg=j;picks[np].depth=r4l_depths[i];np++;
   }
  }
  for(n=1;n<np;n++){
   save=picks[n];z=n;
   while(z>0&&(picks[z-1].depth>save.depth ||
    (picks[z-1].depth==save.depth && picks[z-1].member>save.member))){
    picks[z]=picks[z-1];z--;
   }
   picks[z]=save;
  }
  for(n=0;n<np;n++){
   const Module*m=&ms->v[picks[n].member].m;
   const Segment*s=&m->seg[picks[n].seg];U32 a=alignment(s);
   if(cn==MAX_CONTRIB||!align_(pos,a,&pos)||s->length>R3F_MAX32-pos)return 0;
   contrib[cn].member=picks[n].member;contrib[cn].seg=picks[n].seg+1;
   contrib[cn].kind=cat;contrib[cn].off=pos;contrib[cn].size=s->length;
   contrib[cn].align=a;cn++;pos+=s->length;
  }
  ends[cat]=pos;continue;
 }
#endif /* R4L_LINKER */
 for(i=0;i<ms->n;i++)if(ms->v[i].selected){
  const Module*m=&ms->v[i].m;
  for(j=0;j<m->ns;j++){
   const Segment*seg=&m->seg[j];U32 al=alignment(seg);
   if(category(seg)!=cat)continue;
   if(cn==MAX_CONTRIB||!align_(pos,al,&pos)||seg->length>R3F_MAX32-pos)return 0;
   contrib[cn].member=i;contrib[cn].seg=j+1;contrib[cn].kind=cat;
   contrib[cn].off=pos;contrib[cn].size=seg->length;
   contrib[cn].align=al;cn++;pos+=seg->length;
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
 if(r4l_order_mode){
  /* Original LINK386 symbol table has 256 hash buckets (see FUN_1000_ee94).
   * common_va_ walks backwards, so store descending bucket order here.
   * Names never appear in this policy; ties retain the earlier position. */
  for(i=1;i<kn;i++){
   CommonPlace tmp=commons[i];unsigned h,prev,shift;U32 htmp=0;
   const char *name=tmp.name;size_t len=strlen(name);
   htmp=(U32)len;
   for(h=(unsigned)len;h>0;h--)htmp+=((U32)((U8)name[h-1]|0x20U))<<(h&3U);
   htmp&=255UL;
   j=i;
   while(j>0){
    name=commons[j-1].name;len=strlen(name);prev=(unsigned)len;
    for(shift=(unsigned)len;shift>0;shift--)prev+=((U32)((U8)name[shift-1]|0x20U))<<(shift&3U);
    if((prev&255UL)>=htmp)break;
    commons[j]=commons[j-1];j--;
   }
   commons[j]=tmp;
  }
 }
 for(k=0;k<kn;k++){if(!align_(pos,4,&pos)||commons[k].size>R3F_MAX32-pos)return 0;commons[k].off=pos;pos+=commons[k].size;}ends[2]=pos;return 1;}
