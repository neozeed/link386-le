/* LINK386 R3G: conservative OMF segment-combination planner, ISO C89. */
#include "link386r3f_core.inc"
#define MAX_OUTSEGS 8192
typedef struct {char *name,*cls; unsigned kind,use32,combine,group,owner; char *group_id; U32 size,alignment; } Outseg;
typedef struct {unsigned out; U32 offset;} Placement;
static U32 override_stack=0;
static Outseg os[MAX_OUTSEGS];static Placement placed[MAX_CONTRIB];static unsigned no;
static const char *group_name(const Module*m,const Segment*s){return s->group&&s->group<=m->ng?m->grp[s->group-1].name:"";}
static int mergeable(unsigned co){return co==2||co==4||co==5||co==6||co==7;}
static int combine(const Members*ms){unsigned c,i,j;U32 start,end,at;
 no=0;
 for(c=0;c<3;c++)for(i=0;i<cn;i++)if(contrib[i].kind==c){const Contribution*q=&contrib[i];const Module*m=&ms->v[q->member].m;const Segment*s=&m->seg[q->seg-1];const char*gn=group_name(m,s);int found=-1;
 if(mergeable(s->combine))for(j=0;j<no;j++)if(os[j].kind==c&&os[j].use32==s->use32&&os[j].combine==s->combine&&same_(os[j].name,s->name)&&same_(os[j].cls,s->class_name)&&(same_(os[j].group_id,gn)||
   (same_(s->name,"_DATA")&&same_(s->class_name,"DATA")))){found=(int)j;break;}
 /* Group cross-module comparison is based on the canonical group name stored below. */
 if(found<0){if(no>=MAX_OUTSEGS)return 0;
 j=no++;memset(&os[j],0,sizeof(os[j]));os[j].name=s->name;os[j].cls=s->class_name;os[j].kind=c;os[j].use32=s->use32;os[j].combine=s->combine;os[j].owner=q->member;os[j].group=q->seg;os[j].group_id=(char *)gn;os[j].alignment=q->align;}else j=(unsigned)found;
 if(q->align>os[j].alignment)os[j].alignment=q->align;
 if(!align_(os[j].size,q->align,&at))return 0;
 if(s->combine==6&&found>=0){placed[i].offset=0;}else {placed[i].offset=at;if(q->size>R3F_MAX32-at)return 0;os[j].size=at+q->size;}
 if(s->combine==6&&q->size>os[j].size)os[j].size=q->size;
 placed[i].out=j;
 }
 /* OMF STACK contributions set a minimum; /STACK overrides that allocation.
    Keep contribution offsets intact and enlarge the combined segment tail only. */
 for(j=0;j<no;j++)if(same_(os[j].cls,"STACK")&&override_stack>os[j].size)os[j].size=override_stack;
 /* COMDEF storage is exposed as a separate named BSS output segment, not
    merely an unlabelled appendage to the region. */
 if(kn){U32 common_total=0;for(j=0;j<kn;j++){U32 at;if(!align_(common_total,4,&at)||commons[j].size>R3F_MAX32-at)return 0;common_total=at+commons[j].size;}
 if(no>=MAX_OUTSEGS)return 0;
 j=no++;memset(&os[j],0,sizeof(os[j]));os[j].name="c_common";os[j].cls="BSS";os[j].kind=2;os[j].use32=1;os[j].combine=2;os[j].alignment=16;os[j].size=common_total;}
 for(c=0;c<3;c++){start=0;for(j=0;j<no;j++)if(os[j].kind==c){if(!align_(start,os[j].alignment,&end)||os[j].size>R3F_MAX32-end)return 0;start=end+os[j].size;} }
 return 1;
}
/* R3J: use the class order actually observed in the supplied Microsoft MAP.
   This is a conservative ordering pass: segment identity, size, combination,
   and contribution-relative offsets remain untouched. */
static unsigned class_rank(const Outseg *s){
 const char *c=s->cls, *n=s->name;
 if(s->kind==0)return s->use32?0:1;
 if(s->kind==1){
  if(same_(c,"BEGDATA"))return 0;
  if(same_(c,"DATA")){
   if(same_(n,"_DATA")||same_(n,"DATA"))return 1;
   if(same_(n,"XIB"))return 2;
   if(same_(n,"XI"))return 3;
   if(same_(n,"XIE"))return 4;
   if(same_(n,"XPB"))return 5;
   if(same_(n,"XP"))return 6;
   if(same_(n,"XPE"))return 7;
   if(same_(n,"XCB"))return 8;
   if(same_(n,"XC"))return 9;
   if(same_(n,"XCE"))return 10;
   if(same_(n,"CDATA"))return 11;
   return 12;
  }
  if(same_(c,"CONST"))return 20;
  if(same_(c,"MSG"))return 30;
  return 35;
 }
 if(same_(s->name,"c_common"))return 2;
 if(same_(c,"STACK"))return 3;
 if(same_(c,"BSS"))return 1;
 return 0;
}
static unsigned layout_order[MAX_OUTSEGS];
static void make_layout_order(void){unsigned i,j,t;
 for(i=0;i<no;i++)layout_order[i]=i;
 for(i=1;i<no;i++){
  t=layout_order[i];j=i;
  while(j>0&&(os[layout_order[j-1]].kind>os[t].kind||
    (os[layout_order[j-1]].kind==os[t].kind&&
     class_rank(&os[layout_order[j-1]])>class_rank(&os[t])))){
   layout_order[j]=layout_order[j-1];j--;
  }
  layout_order[j]=t;
 }
}
#ifndef R3N_EMBED
static unsigned group_count(void){unsigned i,j,n=0;for(i=0;i<no;i++)if(os[i].group_id&&*os[i].group_id){for(j=0;j<i;j++)if(same_(os[j].group_id,os[i].group_id))break;if(j==i)n++;}return n;}
static void output(const Members*ms){unsigned i,j,c,n;U32 cur[3]={0,0,0},bases[MAX_OUTSEGS],bss;make_layout_order();puts("LINK386 R3G COMBINED OMF SEGMENT PLAN (NO EXECUTABLE; NOT MICROSOFT-EXACT)");
 for(c=0;c<3;c++){printf("REGION %s\n",c==0?"CODE":c==1?"INITIALIZED":"BSS_STACK");for(n=0;n<no;n++){j=layout_order[n];if(os[j].kind==c){U32 aligned;if(!align_(cur[c],os[j].alignment,&aligned)){puts("INVALID");return;}bases[j]=aligned;cur[c]=aligned+os[j].size;printf("  OUTSEG %u name=%s class=%s use32=%u combine=%u align=%lu offset=%08lX size=%lu\n",j+1,os[j].name,os[j].cls,os[j].use32,os[j].combine,(unsigned long)os[j].alignment,(unsigned long)bases[j],(unsigned long)os[j].size);}}}
 puts("GROUP MEMBERSHIP (identity only; no final LE selectors)");
 for(i=0;i<no;i++)if(os[i].group_id&&*os[i].group_id)printf("  GROUP-MEMBER group=%s outseg=%u name=%s class=%s use32=%u\n",os[i].group_id,i+1,os[i].name,os[i].cls,os[i].use32);
 puts("CONTRIBUTIONS");for(i=0;i<cn;i++){const Contribution*q=&contrib[i];const Module*m=&ms->v[q->member].m;const Segment*s=&m->seg[q->seg-1];j=placed[i].out;printf("  CONTRIBUTION module=%s seg=%s outseg=%u local=%08lX region_offset=%08lX size=%lu\n",m->name?m->name:"?",s->name,j+1,(unsigned long)placed[i].offset,(unsigned long)(bases[j]+placed[i].offset),(unsigned long)q->size);}
 puts("PUBLIC ADDRESSES (REGION RELATIVE)");for(i=0;i<cn;i++){const Contribution*q=&contrib[i];const Module*m=&ms->v[q->member].m;for(n=0;n<m->np;n++)if(m->pub[n].seg==q->seg){const Public*p=&m->pub[n];j=placed[i].out;if(p->offset>q->size){printf("  INVALID-PUBLIC %s\n",p->name);continue;}printf("  PUB %s region=%u off=%08lX module=%s\n",p->name,q->kind,(unsigned long)(bases[j]+placed[i].offset+p->offset),m->name?m->name:"?");}}
 puts("COMMON BSS ALLOCATION (inside c_common output segment)");bss=cur[2];
 for(j=0;j<no;j++)if(same_(os[j].name,"c_common")&&same_(os[j].cls,"BSS")){
 U32 local=0;for(i=0;i<kn;i++){U32 at;if(!align_(local,4,&at)){puts("INVALID COMMON");return;}
 printf("  COMMON %s c_common_offset=%08lX region_offset=%08lX size=%lu\n",commons[i].name,(unsigned long)at,(unsigned long)(bases[j]+at),(unsigned long)commons[i].size);local=at+commons[i].size;}
 }
 printf("BOUNDARY-CANDIDATE _edata=%08lX _end=%08lX (REGION-RELATIVE ONLY)\n",(unsigned long)cur[1],(unsigned long)bss);
 printf("R3G SUMMARY contributions=%u combined_segments=%u commons=%u code=%lu data=%lu bss=%lu groups=%u\n",cn,no,kn,(unsigned long)cur[0],(unsigned long)cur[1],(unsigned long)bss,group_count());
}

#endif
/* R3M: address-space preparation, not an LE writer. The virtual-address
   defaults below are a *policy* and are NOT inferred from the OMF itself. */
#define R3M_MAX_OBJECTS 3
static U32 finalseg[MAX_OUTSEGS];
static unsigned finalobj[MAX_OUTSEGS];
static U32 obj_size[R3M_MAX_OBJECTS];
static U32 obj_base[R3M_MAX_OBJECTS];
static unsigned obj_count;
static int assign_final(void){unsigned i,n,j,o;U32 at;int has16=0;
 for(i=0;i<no;i++)if(os[i].kind==0&&!os[i].use32)has16=1;
 obj_count=has16?3:2;
 obj_base[0]=0x10000UL;obj_base[1]=0x20000UL;obj_base[2]=0x30000UL;
 for(i=0;i<3;i++)obj_size[i]=0;
 for(n=0;n<no;n++){
  j=layout_order[n];o=(os[j].kind==0)?(os[j].use32?0:1):(has16?2:1);
  if(!align_(obj_size[o],os[j].alignment,&at)||os[j].size>R3F_MAX32-at)return 0;
  finalobj[j]=o;finalseg[j]=at;obj_size[o]=at+os[j].size;
 }
 return 1;
}
#ifndef R3N_EMBED
static void output_final(const Members*ms){unsigned i,j,n,o;U32 at;
 if(!assign_final()){puts("R3M ERROR final layout overflow");return;}
 puts("R3M FINAL-ADDRESS CANDIDATES (POLICY BASES; NOT MICROSOFT-EXACT; NO LE EMITTED)");
 for(i=0;i<obj_count;i++)printf("R3M OBJECT index=%u base=%08lX size=%08lX use=%s\n",i+1,(unsigned long)obj_base[i],(unsigned long)obj_size[i],i==0?"CODE32":obj_count==3&&i==1?"CODE16":"DATA");
 for(n=0;n<no;n++){j=layout_order[n];o=finalobj[j];printf("R3M SEGMENT outseg=%u object=%u object_offset=%08lX va=%08lX size=%lu group=%s name=%s\n",j+1,o+1,(unsigned long)finalseg[j],(unsigned long)(obj_base[o]+finalseg[j]),(unsigned long)os[j].size,os[j].group_id?os[j].group_id:"",os[j].name);}
 for(i=0;i<cn;i++){const Contribution*q=&contrib[i];const Module*m=&ms->v[q->member].m;
  j=placed[i].out;o=finalobj[j];at=finalseg[j]+placed[i].offset;
  printf("R3M CONTRIBUTION module=%s seg=%u object=%u offset=%08lX va=%08lX\n",m->name?m->name:"?",q->seg,o+1,(unsigned long)at,(unsigned long)(obj_base[o]+at));
  for(n=0;n<m->np;n++)if(m->pub[n].seg==q->seg){const Public*p=&m->pub[n];if(p->offset>q->size){printf("R3M INVALID-PUBLIC name=%s\n",p->name);continue;}
   printf("R3M SYMBOL name=%s object=%u offset=%08lX va=%08lX\n",p->name,o+1,(unsigned long)(at+p->offset),(unsigned long)(obj_base[o]+at+p->offset));
  }
 }
 for(j=0;j<no;j++)if(same_(os[j].name,"c_common")&&same_(os[j].cls,"BSS")){
 U32 local=0;for(i=0;i<kn;i++){if(!align_(local,4,&at))break;o=finalobj[j];printf("R3M COMMON name=%s object=%u offset=%08lX va=%08lX size=%lu\n",commons[i].name,o+1,(unsigned long)(finalseg[j]+at),(unsigned long)(obj_base[o]+finalseg[j]+at),(unsigned long)commons[i].size);local=at+commons[i].size;}
 }
 puts("R3M GROUP ANCHORS (first listed member; validation only)");
 for(n=0;n<no;n++){j=layout_order[n];if(os[j].group_id&&*os[j].group_id){unsigned k,seen=0;for(k=0;k<n;k++){unsigned prior=layout_order[k];if(same_(os[prior].group_id,os[j].group_id)){seen=1;break;}}if(!seen)printf("R3M GROUP name=%s anchor_object=%u anchor_offset=%08lX\n",os[j].group_id,finalobj[j]+1,(unsigned long)finalseg[j]);}}
 printf("R3M SUMMARY objects=%u segments=%u contributions=%u commons=%u fixups=PREPARATION-ONLY\n",obj_count,no,cn,kn);
}
#endif
#ifndef R3N_EMBED
int main(int argc,char**argv){Members ms;int i,ok=1;unsigned j;memset(&ms,0,sizeof(ms));if(argc<2){fprintf(stderr,"usage: link386r3g file.obj ... file.lib ...\n");return 2;}for(i=1;i<argc;i++){if(!strncmp(argv[i],"--stack=",8)){char*end;unsigned long v=strtoul(argv[i]+8,&end,10);if(!* (argv[i]+8)||*end||v>R3F_MAX32){fprintf(stderr,"R3I: invalid stack size\n");ok=0;}else override_stack=(U32)v;}else if(!input_file(&ms,argv[i]))ok=0;}if(ok){select_members(&ms);if(!place_segments(&ms)||!place_commons(&ms)||!combine(&ms)){fprintf(stderr,"R3G: layout overflow\n");ok=0;}else {make_layout_order();output_final(&ms);output(&ms);}}for(j=0;j<ms.n;j++){free(ms.v[j].file);module_free(&ms.v[j].m);}free(ms.v);return ok?0:1;}

#endif /* R3N_EMBED */
