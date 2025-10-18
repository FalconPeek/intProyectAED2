#ifndef BST_IDX_H
#define BST_IDX_H
#include "cancion.h"

typedef struct nodoB {
    tCancion      info; /* clave: (artista,titulo) */
    struct nodoB* izq;
    struct nodoB* der;
} tBSTI;

void bst_init(tBSTI** A);
int  bst_insert(tBSTI** A, const tCancion* c);     /* 1 ok, 0 duplicado */
tCancion* bst_find(tBSTI* A, const char* artista, const char* titulo);
void bst_inorder(const tBSTI* A);                  /* imprime ordenado */
void bst_free(tBSTI** A);

#endif
