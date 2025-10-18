#ifndef USUARIOS_LISTA_H
#define USUARIOS_LISTA_H

#include "usuario.h"

/* SLL de usuarios */
typedef struct nodoU {
    tUsuario      info;
    struct nodoU* sig;
} tListaU;

void      listaU_init(tListaU** L);
int       listaU_vacia(const tListaU* L);
int       listaU_push_front(tListaU** L, const tUsuario* u);
tUsuario* listaU_find_username(tListaU* L, const char* username);
int       listaU_len(const tListaU* L);
void      listaU_free(tListaU** L);

#endif /* USUARIOS_LISTA_H */
