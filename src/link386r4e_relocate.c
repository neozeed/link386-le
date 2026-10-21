/* LINK386 R4E multi-object relocation pass. Strict C89.
 * Evaluates the supported 16:16, 16:32 and 32-bit OMF FIXUPP forms on
 * freshly assembled source images. Import sites become LE loader records.
 * These source images are not a proof of Windows runtime execution.
 */
#define R4C_LIBRARY_BUILD
#include "link386r4e_images.c"
static U32 rd32_(const U8*p){return (U32)p[0]|((U32)p[1]<<8)|((U32)p[2]<<16)|((U32)p[3]<<24);}
static void wr32_(U8*p,U32 v){p[0]=(U8)v;p[1]=(U8)(v>>8);p[2]=(U8)(v>>16);p[3]=(U8)(v>>24);}
static int contrib_va_(unsigned mi,unsigned seg,U32 *va,unsigned *obj){unsigned q,k;for(q=0;q<cn;q++)if(contrib[q].member==mi&&contrib[q].seg==seg){k=placed[q].out;*obj=finalobj[k];*va=obj_base[*obj]+finalseg[k]+placed[q].offset+(*obj==0?R4C_PREFIX:0);return 1;}return 0;}
static int pub_va_(const Members*ms,const char*sym,U32*va,unsigned*obj,unsigned *imported){unsigned mi,q;*imported=0;
 for(mi=0;mi<ms->n;mi++)if(ms->v[mi].selected){const Module*m=&ms->v[mi].m;
 for(q=0;q<m->np;q++)if((!r4l_order_mode||!m->pub[q].local)&&!strcmp(m->pub[q].name,sym)){
  U32 v;unsigned o;if(m->pub[q].seg&&contrib_va_(mi,m->pub[q].seg,&v,&o)){*va=v+m->pub[q].offset;*obj=o;return 1;}}
 }
 for(mi=0;mi<ms->n;mi++)if(ms->v[mi].selected){const Module*m=&ms->v[mi].m;for(q=0;q<m->ni;q++)if(!strcmp(m->imports[q].internal,sym)){*imported=1;return 1;}}
 return 0;}
/* OMF LEXTDEF (B4/B5) uses module-local LPUBDEF (B6/B7) names.
   Global lookup can select another module's identically named static function. */
static int local_pub_va_(const Members*ms,unsigned mi,const char*sym,U32*va,unsigned*obj,unsigned *imported){
 const Module*m=&ms->v[mi].m;unsigned q;
 *imported=0;
 for(q=0;q<m->np;q++)if(m->pub[q].local&&same_(m->pub[q].name,sym)){
  U32 v;unsigned o;
  if(m->pub[q].seg&&contrib_va_(mi,m->pub[q].seg,&v,&o)){
   *va=v+m->pub[q].offset;*obj=o;return 1;
  }
 }
 return 0;
}
static int group_va_(unsigned mi,unsigned gi,const Members*ms,U32*va,unsigned *obj){const Module*m=&ms->v[mi].m;unsigned k,o;U32 v;if(!gi||gi>m->ng)return 0;
 for(k=0;k<m->grp[gi-1].n;k++)if(contrib_va_(mi,m->grp[gi-1].segments[k],&v,&o)){*va=v;*obj=o;return 1;}
 return 0;}
/* R4D: resolve communal symbols at their true c_common storage locations,
 * not as zero-size public definitions.  This is a source-only calculation. */
static int common_va_(const char*sym,unsigned mi,U32*va,unsigned*obj){
 unsigned j,k;U32 local=0,at;
 for(j=0;j<no;j++)if(same_(os[j].name,"c_common")&&same_(os[j].cls,"BSS")){
  for(k=kn;k>0;){
   k--;
   if(!align_(local,4,&at))return 0;
   if(same_(commons[k].name,sym)&&(!commons[k].local||commons[k].member==mi)){
    *obj=finalobj[j];*va=obj_base[*obj]+finalseg[j]+at+(*obj==0?R4C_PREFIX:0);return 1;
   }
   if(commons[k].size>0xffffffffUL-at)return 0;
   local=at+commons[k].size;
  }
 }
 return 0;
}
/* Boundary symbols are anchored to the final initialized/uninitialized
 * object layout, not to an arbitrary code-group base. */
static int boundary_va_(const char *sym,U32 *va,unsigned *obj){
 unsigned j;U32 mx=0,end;
 if(!same_(sym,"_edata")&&!same_(sym,"_end"))return 0;
 *obj=obj_count-1;
 if(same_(sym,"_end")){
  for(j=0;j<no;j++)if(finalobj[j]==*obj&&same_(os[j].cls,"STACK")){
    *va=obj_base[*obj]+finalseg[j];return 1;
  }
  return 0;
 }
 for(j=0;j<no;j++)if(finalobj[j]==*obj&&os[j].kind==1){
  end=finalseg[j]+os[j].size;if(end>mx)mx=end;
 }
 *va=obj_base[*obj]+mx;return 1;
}
/* DEF and OMF import declarations both contribute to the loader module table. */
#define R4E_MAX_MODULES 32
static char r4e_dll_names[R4E_MAX_MODULES][64];
static unsigned r4e_dll_count=0, r4e_last_dll=0;
static unsigned r4e_register_dll_(const char*dll){unsigned i;
 for(i=0;i<r4e_dll_count;i++)if(same_(dll,r4e_dll_names[i]))return i+1;
 if(r4e_dll_count>=R4E_MAX_MODULES||strlen(dll)>=sizeof(r4e_dll_names[0]))return 0;
 strcpy(r4e_dll_names[r4e_dll_count],dll);return ++r4e_dll_count;
}
static int import_ordinal_(const Members*ms,const char*sym,unsigned*ord){unsigned mi,k;
 for(mi=0;mi<ms->n;mi++)if(ms->v[mi].selected){const Module*m=&ms->v[mi].m;
  for(k=0;k<m->ni;k++)if(same_(m->imports[k].internal,sym)&&m->imports[k].byordinal){
    *ord=m->imports[k].ordinal;
    r4e_last_dll=r4e_register_dll_(m->imports[k].dll);return r4e_last_dll!=0;
  }
 }
 return 0;
}
/* Every independently derived LE loader fixup is retained in memory
 * for the source-only R4D emitter.  For LE hello, 159 absolute offsets are
 * loader relocations and 12 imports are REL32 DLL relocations. */
#define R4D_MAX_LOADER_SITES 65536U
typedef struct {unsigned obj,mode,target_obj,ordinal,loc,module;U32 site,target_off;} LoaderSite;
static LoaderSite loader_sites[R4D_MAX_LOADER_SITES];
static unsigned loader_count=0;
static unsigned r4e_current_loc=9;
static int loader_add_(unsigned obj,U32 site,unsigned mode,unsigned target_obj,U32 target_off,unsigned ordinal){
 LoaderSite*x;
 if(loader_count>=R4D_MAX_LOADER_SITES)return 0;
 x=&loader_sites[loader_count++];x->obj=obj;x->site=site;x->mode=mode;x->target_obj=target_obj;x->target_off=target_off;x->ordinal=ordinal;x->loc=r4e_current_loc;x->module=(mode?r4e_last_dll:0);
 return 1;
}
static int apply_(const Members*ms,const char*dir){
 char path[1024];FILE*rep;unsigned mi,j,n,ns,ready=0,imports=0,deferred=0,total=0,absolute=0,relative=0,expanded=0;
 U32 target=0,site,srcbase,initial,val;unsigned ob,destob,imp,ord;
 if(strlen(dir)+50>=sizeof(path))return 0;
 sprintf(path,"%s/relocation-candidates-r4d.tsv",dir);rep=fopen(path,"wb");if(!rep)return 0;
 fputs("module\tsegment\tsite\tmode\ttarget_method\ttarget_idx\tresult\tvalue\ttarget\tordinal\n",rep);
 for(mi=0;mi<ms->n;mi++)if(ms->v[mi].selected){const Module*m=&ms->v[mi].m;
  for(j=0;j<m->nf;j++){
   const Fixup*f=&m->fix[j];const Data*d=f->data_id&&f->data_id<=m->nd?&m->data[f->data_id-1]:NULL;
   const char*reason=NULL,*sym="";total++;target=0;imp=0;destob=0;ord=0;
   if(!d||!f->seg||!contrib_va_(mi,f->seg,&srcbase,&ob)){reason="SOURCE_INVALID";goto defer;}
   if(f->loc!=9&&f->loc!=11&&f->loc!=3&&f->loc!=2){reason="WIDTH_UNSUPPORTED";goto defer;}
   r4e_current_loc=f->loc;
   if(!fixup_source_sites(d,f->at,f->loc==11?6:(f->loc==3?4:(f->loc==2?2:4)),r4d_fixsites,R4D_FIXUP_SITE_CAP,&ns)){
    reason="ENCODED_FIXUP_SITE_INVALID";goto defer;
   }
   if(f->target_method==0){if(!contrib_va_(mi,f->target_idx,&target,&destob))reason="TARGET_SEGMENT_INVALID";}
   else if(f->target_method==1){if(!group_va_(mi,f->target_idx,ms,&target,&destob))reason="TARGET_GROUP_UNRESOLVED";}
   else if(f->target_method==2){
    if(!f->target_idx||f->target_idx>m->ne){reason="TARGET_EXTERNAL_INDEX_INVALID";}
    else{
     sym=m->ext[f->target_idx-1].name;
     if(!(r4l_order_mode && m->ext[f->target_idx-1].local &&
           local_pub_va_(ms,mi,sym,&target,&destob,&imp)) &&
        !pub_va_(ms,sym,&target,&destob,&imp)&&
        !common_va_(sym,mi,&target,&destob)&&
        !boundary_va_(sym,&target,&destob))reason="SYMBOL_UNRESOLVED";
     if(imp&&!import_ordinal_(ms,sym,&ord))reason="IMPORT_UNSUPPORTED";
    }
   }else reason="TARGET_METHOD_UNSUPPORTED";
   if(reason)goto defer;
   for(n=0;n<ns;n++){
    site=srcbase+d->offset+r4d_fixsites[n];
    if(site<obj_base[ob]||site>obj_base[ob]+imgs[ob].length||(f->loc==11?6UL:(f->loc==3?4UL:(f->loc==2?2UL:4UL)))>obj_base[ob]+imgs[ob].length-site){
     reason="SOURCE_OUT_OF_BOUNDS";goto defer;
    }
   }
   expanded+=ns;
   for(n=0;n<ns;n++){
    site=srcbase+d->offset+r4d_fixsites[n];
    if(imp){imports++;
     if(!loader_add_(ob,site,1,0,0,ord))return 0;
     fprintf(rep,"%s\t%u\t%08lX\t%u\t%u\t%u\tIMPORT_DEFERRED\t-\t%s\t%u\n",m->name,f->seg,(unsigned long)site,f->mode,f->target_method,f->target_idx,sym,ord);
     continue;
    }
    if(f->loc!=9){
     /* Segment base (loc2), 16:16 pointer (loc3), 16:32 pointer (loc11).
      * These may need loader selector relocation; do not interpret as REL32. */
     U32 targetoff=target-obj_base[destob]+(f->has_disp?f->disp:0);
     if(!f->mode||target<obj_base[destob]||targetoff>=imgs[destob].length){reason="MIXED_TARGET_INVALID";goto defer;}
     if(f->loc==11)wr32_(imgs[ob].data+(site-obj_base[ob]),target);
     if(f->loc==3){imgs[ob].data[site-obj_base[ob]]=(U8)targetoff;
       imgs[ob].data[site-obj_base[ob]+1]=(U8)(targetoff>>8);}
     if(!loader_add_(ob,site,0,destob+1,targetoff,0))return 0;
     ready++;absolute++;
     fprintf(rep,"%s\t%u\t%08lX\t%u\t%u\t%u\tMIXED_LOADER\t%08lX\t%s\t0\n",m->name,f->seg,(unsigned long)site,f->mode,f->target_method,f->target_idx,(unsigned long)targetoff,sym);
     continue;
    }
    initial=rd32_(imgs[ob].data+(site-obj_base[ob]));
    val=target+(f->has_disp?f->disp:0)+initial;
    if(!f->mode)val-=site+4;
    /* OMF frame=GROUP(DGROUP) requests a group-relative offset. The
     * group is rooted at the data object base in this C/386 memory model.
     * These fields are statically resolved, not LE loader OFF32 sites. */
    else if(f->frame_method==1&&f->frame_idx&&f->frame_idx<=m->ng&&
       same_(m->grp[f->frame_idx-1].name,"DGROUP")&&destob==obj_count-1)
     val-=obj_base[destob];
    /* FIXUPP frame=EXT(target) denotes an offset in the target's own
       segment/object, not a preferred linear VA. The six cmiscdat LIDATA
       replicates use this form and are not LE loader relocation sites. */
    else if(f->frame_method==2&&f->target_method==2&&f->frame_idx==f->target_idx)
     val-=obj_base[destob];
    /* The OMF field is a 32-bit i386 value, even when unsigned long is 64-bit. */
    val&=0xffffffffUL;
    wr32_(imgs[ob].data+(site-obj_base[ob]),val);
    if(f->mode && !(f->frame_method==2&&f->target_method==2&&f->frame_idx==f->target_idx)&&
       !(f->frame_method==1&&f->frame_idx&&f->frame_idx<=m->ng&&
         same_(m->grp[f->frame_idx-1].name,"DGROUP")&&destob==obj_count-1)){
     if(val<obj_base[destob]||!loader_add_(ob,site,0,destob+1,val-obj_base[destob],0))return 0;
    }
    ready++;if(f->mode)absolute++;else relative++;
    fprintf(rep,"%s\t%u\t%08lX\t%u\t%u\t%u\tCANDIDATE\t%08lX\t%s\t0\n",m->name,f->seg,(unsigned long)site,f->mode,f->target_method,f->target_idx,(unsigned long)(val&0xffffffffUL),sym);
   }
   continue;
 defer:
   deferred++;
   fprintf(rep,"%s\t%u\t-\t%u\t%u\t%u\t%s\t-\t%s\t0\n",m->name,f->seg,f->mode,f->target_method,f->target_idx,reason?reason:"UNKNOWN",sym);
  }
 }
 if(fclose(rep))return 0;
 for(j=0;j<obj_count;j++)if(!save_image(dir,j,"r4d-candidate-relocated",imgs[j].data,imgs[j].length))return 0;
 printf("R4E RELOCATION-CANDIDATES input_fixups=%u expanded_sites=%u candidate_applied=%u absolute=%u relative=%u import_deferred=%u other_deferred=%u verified_against_reference=NO runnable_le=NO\n",total,expanded,ready,absolute,relative,imports,deferred);
 return 1;
}
#ifndef R4D_LIB_BUILD
int main(int argc,char**argv){Members ms;int i,ok=1;unsigned j;memset(&ms,0,sizeof(ms));if(argc!=5){fprintf(stderr,"usage: link386r4c_relocate OUTPUT_DIR hi.obj LIBC.LIB OS2386.LIB\n");return 2;}
 for(i=2;i<5;i++)if(!input_file(&ms,argv[i]))ok=0;
 if(ok){select_members(&ms);if(!place_segments(&ms)||!place_commons(&ms)||!combine(&ms))ok=0;else{make_layout_order();if(!assign_final()||!images_build(&ms)||!apply_(&ms,argv[1]))ok=0;}}
 for(j=0;j<obj_count;j++){free(imgs[j].data);free(imgs[j].mask);}for(j=0;j<ms.n;j++){free(ms.v[j].file);module_free(&ms.v[j].m);}free(ms.v);return ok?0:1;}

#endif /* R4D_LIB_BUILD */
