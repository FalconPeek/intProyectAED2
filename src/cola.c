/* Cola de reproducción (FIFO) — src/cola.c */
#include "cola.h"
#include <stdlib.h>

void cola_init(tColaQ* Q){
    Q->pri = NULL;
    Q->fin = NULL;
}

int  cola_vacia(const tColaQ* Q){
    return Q->pri == NULL;
}

int  cola_enq(tColaQ* Q, tPlayItem x){
    tColaN* n = (tColaN*)malloc(sizeof(*n));
    if(!n) return 0;
    n->info = x;
    n->sig  = NULL;
    if(Q->fin == NULL){           /* cola vacía */
        Q->pri = Q->fin = n;
    }else{
        Q->fin->sig = n;
        Q->fin      = n;
    }
    return 1;
}

int  cola_deq(tColaQ* Q, tPlayItem* out){
    if(Q->pri == NULL) return 0;
    tColaN* n = Q->pri;
    if(out) *out = n->info;
    Q->pri = n->sig;
    if(Q->pri == NULL) Q->fin = NULL; /* quedó vacía */
    free(n);
    return 1;
}

void cola_free(tColaQ* Q){
    tPlayItem tmp;
    while(cola_deq(Q, &tmp));
}
