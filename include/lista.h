#ifndef LISTA_H
#define LISTA_H
#include "cancion.h"

/* Lista simplemente enlazada para canciones */
typedef struct nodoL {
    tCancion      info;
    struct nodoL* sig;
} tListaC;

void lista_init(tListaC** L);                         /* *L=NULL */
int  lista_vacia(const tListaC* L);
int  lista_push_front(tListaC** L, tCancion x);        /* 1 ok, 0 sin memoria */
tCancion* lista_find_by_title(tListaC* L, const char* titulo);
void lista_free(tListaC** L);

/* Utilidades */
int  lista_len(const tListaC* L);
int  lista_to_array(const tListaC* L, tCancion* v, int nmax); /* copia hasta nmax */

#endif
