/* R4E: source-only CMD32 multi-object layout/materialization probe. */
#define R4C_LIBRARY_BUILD
#include "link386r4e_images.c"
#include "link386r4e_def.inc"
int main(int argc,char**argv){Members ms;int i,ok=1;unsigned j,sel=0,imps=0,missing=0;U32 total=0;
 memset(&ms,0,sizeof ms);
 if(argc<7){fputs("usage: r4e_probe OUTDIR OBJ... LIBC.LIB OS2386.LIB\n",stderr);return 2;}
 for(i=3;i<argc;i++)if(!input_file(&ms,argv[i]))ok=0;
 if(ok && !r4e_def_imports(&ms,argv[2]))ok=0;
 if(ok){select_members(&ms);for(j=0;j<ms.n;j++)if(ms.v[j].selected){Module*m=&ms.v[j].m;unsigned k;sel++;for(k=0;k<m->ni;k++)imps++;for(k=0;k<m->ne;k++)if(!selected_export(&ms,m->ext[k].name)){if(missing<35)printf("R4E UNRESOLVED module=%s symbol=%s\n",m->name,m->ext[k].name);missing++;}}
 printf("R4E SELECTED modules=%u imports=%u unresolved_occurrences=%u\n",sel,imps,missing);
 {unsigned counts[16]={0}, modes[16]={0},q,k;for(q=0;q<ms.n;q++)if(ms.v[q].selected)for(k=0;k<ms.v[q].m.nf;k++){const Fixup *f=&ms.v[q].m.fix[k]; if(f->loc<16){counts[f->loc]++;if(f->mode)modes[f->loc]++;}else printf("R4E odd_loc %u\n",f->loc);}for(q=0;q<ms.n;q++)if(ms.v[q].selected)for(k=0;k<ms.v[q].m.nf;k++){const Fixup *f=&ms.v[q].m.fix[k];if(f->loc!=9)printf("R4E SPECIAL module=%s seg=%u loc=%u at=%u fmode=%u target_method=%u target_idx=%u frame_method=%u frame_idx=%u disp=%08lX\n",ms.v[q].m.name,f->seg,f->loc,f->at,f->mode,f->target_method,f->target_idx,f->frame_method,f->frame_idx,(unsigned long)f->disp);}for(q=0;q<16;q++)if(counts[q])printf("R4E LOCATION loc=%u total=%u segment=%u self_relative=%u\n",q,counts[q],modes[q],counts[q]-modes[q]);}

 if(!place_segments(&ms)||!place_commons(&ms)||!combine(&ms))ok=0;
 else{make_layout_order();if(!assign_final())ok=0;else {for(j=0;j<obj_count;j++){printf("R4E PRE-OBJECT obj=%u base=%08lX extent=%08lX\n",j+1,(unsigned long)obj_base[j],(unsigned long)obj_size[j]);total+=obj_size[j];}printf("R4E PRE-IMAGE total=%lu contributions=%u combined=%u commons=%u\n",(unsigned long)total,cn,no,kn);
 {unsigned s,v;for(s=0;s<ms.n;s++)if(ms.v[s].selected){const Module*m=&ms.v[s].m;for(v=0;v<m->np;v++)if(strstr(m->pub[v].name,"start")||strstr(m->pub[v].name,"START")||strstr(m->pub[v].name,"main"))printf("R4E ENTRY_CANDIDATE %s %s seg=%u off=%lu\n",m->name,m->pub[v].name,m->pub[v].seg,(unsigned long)m->pub[v].offset);}}if(!images_build(&ms)){fprintf(stderr,"R4E MATERIALIZATION FAILED\n");ok=0;}else {for(j=0;j<obj_count;j++)if(!save_image(argv[1],j,"r4e-prelim",imgs[j].data,imgs[j].length))ok=0;}}}
 }
 for(j=0;j<obj_count;j++){free(imgs[j].data);free(imgs[j].mask);}for(j=0;j<ms.n;j++){free(ms.v[j].file);module_free(&ms.v[j].m);}free(ms.v);return ok?0:1;
}
