#define R4D_LIB_BUILD
#include "link386r4e_relocate.c"
#include "link386r4e_def.inc"
int main(int argc,char**argv){Members ms;int i,ok=1;unsigned j;memset(&ms,0,sizeof ms);
 if(argc<7){fputs("usage: applyprobe OUTDIR DEF OBJ... LIBC.LIB OS2386.LIB\n",stderr);return 2;}
 for(i=3;i<argc;i++)if(!input_file(&ms,argv[i]))ok=0;
 if(ok&&!r4e_def_imports(&ms,argv[2]))ok=0;
 if(ok){select_members(&ms);if(!place_segments(&ms)||!place_commons(&ms)||!combine(&ms))ok=0;
 else {make_layout_order();if(!assign_final()||!images_build(&ms)||!apply_(&ms,argv[1]))ok=0;}}
 for(j=0;j<obj_count;j++){free(imgs[j].data);free(imgs[j].mask);}for(j=0;j<ms.n;j++){free(ms.v[j].file);module_free(&ms.v[j].m);}free(ms.v);return ok?0:1;
}
