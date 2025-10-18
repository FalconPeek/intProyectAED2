#include "persist_usuarios_txt.h"
#include <stdio.h>
#include <errno.h>

int persist_usuarios_load_txt(const char* path, tListaU** L){
    FILE* f=fopen(path,"r");
    if(!f) return (errno==ENOENT)?1:0;
    tUsuario u;
    int r=fscanf(f," %49s %ld %d", u.username,
                 &u.segundos_escucha_total, &u.reproducciones_totales);
    while(!feof(f)){
        if(r==3){
            if(!listaU_push_front(L,&u)){ fclose(f); return 0; }
        }
        r=fscanf(f," %49s %ld %d", u.username,
                 &u.segundos_escucha_total, &u.reproducciones_totales);
    }
    fclose(f); return 1;
}

int persist_usuarios_save_txt(const char* path, const tListaU* L){
    FILE* f=fopen(path,"w"); if(!f) return 0;
    for(; L; L=L->sig){
        if(fprintf(f,"%s %ld %d\n",
                   L->info.username,
                   L->info.segundos_escucha_total,
                   L->info.reproducciones_totales)<=0){ fclose(f); return 0; }
    }
    fclose(f); return 1;
}
