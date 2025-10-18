/* src/bst_idx.c
   Índice ABB por (artista, título).
   - Inserción/búsqueda recursivas
   - In-orden para depuración
   - Liberación postorden
*/
#include "bst_idx.h"
#include "cancion.h"   /* cancion_print, comparadores */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Comparador de clave: (artista, titulo) */
static int cmp_artista_titulo(const tCancion* a, const tCancion* b){
    int c = strcmp(a->artista, b->artista);
    if(c != 0) return c;
    return strcmp(a->titulo, b->titulo);
}

void bst_init(tBSTI** A){
    *A = NULL;
}

/* Inserta por copia de valor. Retorna 1 si ok, 0 si duplicado o sin memoria. */
int bst_insert(tBSTI** A, const tCancion* c){
    if(*A == NULL){
        tBSTI* n = (tBSTI*)malloc(sizeof(*n));
        if(!n) return 0;
        n->info = *c;      /* copia por valor */
        n->izq  = NULL;
        n->der  = NULL;
        *A = n;
        return 1;
    }
    int cmp = cmp_artista_titulo(c, &(*A)->info);
    if(cmp < 0)  return bst_insert(&(*A)->izq, c);
    if(cmp > 0)  return bst_insert(&(*A)->der, c);
    /* cmp == 0 -> duplicado exacto de clave: no insertamos */
    return 0;
}

/* Busca nodo por (artista,titulo). Retorna puntero a la canción (en árbol) o NULL. */
tCancion* bst_find(tBSTI* A, const char* artista, const char* titulo){
    if(!A) return NULL;
    int c = strcmp(artista, A->info.artista);
    if(c == 0){
        int t = strcmp(titulo, A->info.titulo);
        if(t == 0) return &A->info;
        if(t < 0)  return bst_find(A->izq, artista, titulo);
        else       return bst_find(A->der, artista, titulo);
    }
    if(c < 0) return bst_find(A->izq, artista, titulo);
    return bst_find(A->der, artista, titulo);
}

/* Recorre e imprime en-orden (alfabético por artista,título) para depurar. */
void bst_inorder(const tBSTI* A){
    if(!A) return;
    bst_inorder(A->izq);
    cancion_print(&A->info);
    bst_inorder(A->der);
}

/* Libera todo el árbol (postorden). */
void bst_free(tBSTI** A){
    if(!*A) return;
    bst_free(&(*A)->izq);
    bst_free(&(*A)->der);
    free(*A);
    *A = NULL;
}
