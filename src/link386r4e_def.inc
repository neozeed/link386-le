/* R4E: strict subset of OS/2 LINK386 module-definition IMPORTS.
 * Input syntax: IMPORTS followed by internal=MODULE.ordinal, whitespace or
 * semicolon terminates; declarations in the original DEF stay authoritative.
 * This creates OMF-equivalent imported symbol declarations in a synthetic
 * selected module; it does not assume a missing static VIO/KBD library. */
static int r4e_def_imports(Members*ms,const char*path){FILE*fp;Module m;char line[512],a[256],dll[128];unsigned ordinal,n=0;int section=0;
 memset(&m,0,sizeof(m));fp=fopen(path,"rb");if(!fp){perror(path);return 0;}
 m.name=dup_("R4E_DEF_IMPORTS");if(!m.name){fclose(fp);return 0;}
 while(fgets(line,sizeof line,fp)){
  char*p=line,*q,*dot,*equals,*end;
  while(*p==' '||*p=='\t')p++;
  if(*p=='\r'||*p=='\n'||*p==';'||!*p)continue;
  if(!strncmp(p,"IMPORTS",7)&&(p[7]=='\r'||p[7]=='\n'||p[7]==' '||p[7]=='\t')){section=1;continue;}
  if(!section)continue;
  if(*p<'A'||*p>'z')continue;
  if(!strchr(p,'=')){section=0;continue;}
  equals=strchr(p,'=');dot=strchr(equals+1,'.');if(!dot){section=0;continue;}
  q=equals;while(q>p&&(q[-1]==' '||q[-1]=='\t'))q--;
  if((size_t)(q-p)>=sizeof a||(size_t)(dot-equals-1)>=sizeof dll){fclose(fp);module_free(&m);return 0;}
  memcpy(a,p,(size_t)(q-p));a[q-p]=0;
  q=equals+1;while(q<dot&&(*q==' '||*q=='\t'))q++;
  memcpy(dll,q,(size_t)(dot-q));dll[dot-q]=0;
  ordinal=(unsigned)strtoul(dot+1,&end,10);
  while(*end==' '||*end=='\t'||*end=='\r'||*end=='\n')end++;
  if(!a[0]||!dll[0]||!ordinal||ordinal>65535||(*end&&*end!=';')){fprintf(stderr,"R4E: malformed DEF import: %s",line);fclose(fp);module_free(&m);return 0;}
  if(!grow((void**)&m.imports,&m.ci,m.ni+1,sizeof(Import))){fclose(fp);module_free(&m);return 0;}
  {Import*imp=&m.imports[m.ni++];memset(imp,0,sizeof(*imp));imp->internal=dup_(a);imp->dll=dup_(dll);imp->byordinal=1;imp->ordinal=ordinal;
  if(!imp->internal||!imp->dll){fclose(fp);module_free(&m);return 0;}}
  n++;
 }
 fclose(fp);
 if(!put_member(ms,path,0,0,&m)){module_free(&m);return 0;}
 printf("R4E DEF IMPORTS file=%s count=%u\n",path,n);
 return 1;
}
