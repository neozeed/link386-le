/* R3N bounded relocation evaluator. Planner-only; no writeback to LE. */
#define R3N_EMBED
#include "link386r3g.c"
static int contribution(unsigned member,unsigned seg,U32 *va,unsigned *obj){unsigned k,j;for(k=0;k<cn;k++)if(contrib[k].member==member&&contrib[k].seg==seg){j=placed[k].out;*obj=finalobj[j]+1;*va=obj_base[finalobj[j]]+finalseg[j]+placed[k].offset;return 1;}return 0;}
static int resolve_symbol(const Members *ms,const char *name,U32 *va,unsigned *obj,unsigned *imported){unsigned i,j,k;U32 b;unsigned o;
 *imported=0;
 for(i=0;i<ms->n;i++)if(ms->v[i].selected){const Module*m=&ms->v[i].m;for(j=0;j<m->np;j++)if(!strcmp(m->pub[j].name,name)&&m->pub[j].seg&&contribution(i,m->pub[j].seg,&b,&o)){*va=b+m->pub[j].offset;*obj=o;return 1;}}
 for(k=0;k<kn;k++)if(!strcmp(commons[k].name,name)){for(j=0;j<no;j++)if(same_(os[j].name,"c_common")){U32 local=0;unsigned n;for(n=0;n<k;n++){if(!align_(local,4,&local))return 0;local+=commons[n].size;}if(!align_(local,4,&local))return 0;*obj=finalobj[j]+1;*va=obj_base[finalobj[j]]+finalseg[j]+local;return 1;}}
 for(i=0;i<ms->n;i++)if(ms->v[i].selected){const Module*m=&ms->v[i].m;for(j=0;j<m->ni;j++)if(!strcmp(m->imports[j].internal,name)){*imported=1;*obj=0;*va=0;return 1;}}
 return 0;}
static void classify(const Members *ms){unsigned i,j,local=0,imports=0,blocked=0,bad=0,total=0;puts("R3N RELOCATION CLASSIFICATION (NO IMAGE EMITTED)");
 for(i=0;i<ms->n;i++)if(ms->v[i].selected){const Module*m=&ms->v[i].m;for(j=0;j<m->nf;j++){const Fixup*f=&m->fix[j];const Data*d=f->data_id&&f->data_id<=m->nd?&m->data[f->data_id-1]:NULL;U32 src=0,dst=0,at=0,addend=0;unsigned srcobj=0,dstobj=0,imp=0,bytes=0;const char *why="";int known=0;total++;
 if(!d||!f->seg||!contribution(i,f->seg,&src,&srcobj)){why="SOURCE_UNPLACED";goto failed;}
 if(f->loc==9||f->loc==11)bytes=4;else if(f->loc==1||f->loc==2||f->loc==3)bytes=2;else if(f->loc==0)bytes=1;else {why="LOCATION_UNSUPPORTED";goto failed;}
 if(f->at>d->length||bytes>d->length-f->at){why="SITE_OUTSIDE_DATA";goto failed;}
 if(d->offset>m->seg[f->seg-1].length||f->at>m->seg[f->seg-1].length-d->offset){why="SITE_OUTSIDE_SEGMENT";goto failed;}
 at=src+d->offset+f->at;
 if(bytes==4)addend=(U32)d->bytes[f->at]|((U32)d->bytes[f->at+1]<<8)|((U32)d->bytes[f->at+2]<<16)|((U32)d->bytes[f->at+3]<<24);
 else if(bytes==2)addend=(U32)d->bytes[f->at]|((U32)d->bytes[f->at+1]<<8);
 else addend=d->bytes[f->at];
 if(f->target_method==0){known=contribution(i,f->target_idx,&dst,&dstobj);if(!known)why="TARGET_SEGMENT_INVALID";}
 else if(f->target_method==1){unsigned k;if(f->target_idx&&f->target_idx<=m->ng){const Group*g=&m->grp[f->target_idx-1];for(k=0;k<g->n;k++)if(contribution(i,g->segments[k],&dst,&dstobj)){known=1;break;}}if(!known)why="TARGET_GROUP_INVALID";}
 else if(f->target_method==2){if(f->target_idx&&f->target_idx<=m->ne){known=resolve_symbol(ms,m->ext[f->target_idx-1].name,&dst,&dstobj,&imp);if(!known)why="TARGET_EXTERNAL_UNRESOLVED";}else why="TARGET_EXTERNAL_INDEX_INVALID";}
 else why="TARGET_METHOD_UNSUPPORTED";
 if(!known)goto failed;
 if(imp){imports++;printf("R3N IMPORT module=%s site=%08lX loc=%u symbol=%s\n",m->name?m->name:"?",(unsigned long)at,f->loc,f->target_method==2?m->ext[f->target_idx-1].name:"?");continue;}
 /* These values are diagnostics only: frame method and displacement semantics
    must be validated before using them for image writeback. */
 if(bytes==4&&f->loc==9){U32 preliminary=dst+f->disp+addend;if(!f->mode)preliminary-=at+4;printf("R3N CANDIDATE module=%s site=%08lX object=%u target=%08lX target_object=%u disp=%08lX addend=%08lX value=%08lX mode=%u frame=%u:%u\n",m->name?m->name:"?",(unsigned long)at,srcobj,(unsigned long)dst,dstobj,(unsigned long)f->disp,(unsigned long)addend,(unsigned long)preliminary,f->mode,f->frame_method,f->frame_idx);local++;}
 else{blocked++;printf("R3N DEFER module=%s site=%08lX loc=%u reason=FRAME_OR_WIDTH_UNVALIDATED\n",m->name?m->name:"?",(unsigned long)at,f->loc);}
 continue;
 failed:bad++;printf("R3N BLOCK module=%s fixup=%u reason=%s method=%u loc=%u extidx=%u extcount=%u name=%s dataid=%u dataoffset=%lu datalen=%lu at=%u\n",m->name?m->name:"?",j+1,why,f->target_method,f->loc,f->target_idx,m->ne,(f->target_method==2&&f->target_idx&&f->target_idx<=m->ne)?m->ext[f->target_idx-1].name:"?",f->data_id,d?(unsigned long)d->offset:0,d?(unsigned long)d->length:0,f->at);
 }}
 printf("R3N SUMMARY total=%u candidate_offset32=%u imports=%u deferred=%u blocked=%u writebacks=0\n",total,local,imports,blocked,bad);
}
int main(int argc,char **argv){Members ms;int i,ok=1;unsigned j;memset(&ms,0,sizeof(ms));if(argc<2){fprintf(stderr,"usage: link386r3n [--stack=BYTES] OBJ... LIB...\n");return 2;}for(i=1;i<argc;i++)if(!strncmp(argv[i],"--stack=",8)){char *end;unsigned long v=strtoul(argv[i]+8,&end,10);if(!*(argv[i]+8)||*end||v>R3F_MAX32)ok=0;else override_stack=(U32)v;}else if(!input_file(&ms,argv[i]))ok=0;
 if(ok){select_members(&ms);if(!place_segments(&ms)||!place_commons(&ms)||!combine(&ms)){ok=0;}else{make_layout_order();if(!assign_final())ok=0;else classify(&ms);}}
 for(j=0;j<ms.n;j++){free(ms.v[j].file);module_free(&ms.v[j].m);}free(ms.v);return ok?0:1;}
