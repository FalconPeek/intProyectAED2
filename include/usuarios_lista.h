#ifndef USUARIOS_LISTA_H
#define USUARIOS_LISTA_H

#include "usuario.h"

/* Lista simplemente enlazada de usuarios */
typedef struct nodoU {
    tUsuario      info;
    struct nodoU* sig;
} tListaU;

void listaU_init(tListaU** L);                       /* *L = NULL */
int  listaU_vacia(const tListaU* L);                 /* 1 si vacía */
int  listaU_push_front(tListaU** L, const tUsuario* u); /* 1 ok, 0 sin mem */
tUsuario* listaU_find_username(tListaU* L, const char* username); /* NULL si no está */
int  listaU_len(const tListaU* L);
void listaU_free(tListaU** L);

#endif /* USUARIOS_LISTA_H */
