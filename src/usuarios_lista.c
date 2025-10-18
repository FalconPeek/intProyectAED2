#include "usuarios_lista.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void listaU_init(tListaU** L){ *L = NULL; }
int  listaU_vacia(const tListaU* L){ return L == NULL; }

int  listaU_push_front(tListaU** L, const tUsuario* u){
    tListaU* n = (tListaU*)malloc(sizeof(*n));
    if(!n) return 0;
    n->info = *u;
    n->sig  = *L;
    *L = n;
    return 1;
}

tUsuario* listaU_find_username(tListaU* L, const char* username){
    for(; L; L=L->sig){
        if(strcmp(L->info.username, username)==0) return &L->info;
    }
    return NULL;
}

int listaU_len(const tListaU* L){
    int c=0; while(L){ c++; L=L->sig; } return c;
}

void listaU_free(tListaU** L){
    while(*L){ tListaU* x=*L; *L=(*L)->sig; free(x); }
}

void usuario_print(const tUsuario* u){
    if(!u){ puts("(usuario NULL)"); return; }
    printf("Usuario: %-20s  plays=%d  seg=%ld\n",
           u->username, u->reproducciones_totales, u->segundos_escucha_total);
}
