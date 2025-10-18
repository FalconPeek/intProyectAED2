/* Lista simplemente enlazada de canciones — src/lista.c */
#include "lista.h"
#include <stdlib.h>
#include <string.h>

void lista_init(tListaC** L){ *L = NULL; }

int  lista_vacia(const tListaC* L){ return L == NULL; }

int  lista_push_front(tListaC** L, tCancion x){
    tListaC* n = (tListaC*)malloc(sizeof(*n));
    if(!n) return 0;
    n->info = x;         /* copia por valor */
    n->sig  = *L;
    *L = n;
    return 1;
}

tCancion* lista_find_by_title(tListaC* L, const char* titulo){
    for(; L; L = L->sig){
        if(strcmp(L->info.titulo, titulo) == 0)
            return &L->info;
    }
    return NULL;
}

int lista_len(const tListaC* L){
    int c = 0;
    while(L){ c++; L = L->sig; }
    return c;
}

int  lista_to_array(const tListaC* L, tCancion* v, int nmax){
    int i = 0;
    while(L && i < nmax){
        v[i++] = L->info;  /* copia por valor */
        L = L->sig;
    }
    return i; /* cantidad copiada */
}

void lista_free(tListaC** L){
    while(*L){
        tListaC* x = *L;
        *L = (*L)->sig;
        free(x);
    }
}
