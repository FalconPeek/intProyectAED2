/* Pila de acciones (LIFO) — src/pila.c */
#include "pila.h"
#include <stdlib.h>

void pila_init(tPilaA** P){ *P = NULL; }

int  pila_vacia(const tPilaA* P){ return P == NULL; }

int  pila_push(tPilaA** P, tAccion x){
    tPilaA* n = (tPilaA*)malloc(sizeof(*n));
    if(!n) return 0;
    n->info = x;
    n->sig  = *P;
    *P = n;
    return 1;
}

int  pila_pop(tPilaA** P, tAccion* out){
    if(!*P) return 0;
    tPilaA* n = *P;
    if(out) *out = n->info;
    *P = n->sig;
    free(n);
    return 1;
}

void pila_free(tPilaA** P){
    tAccion tmp;
    while(pila_pop(P, &tmp));
}
