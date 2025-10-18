#ifndef PILA_H
#define PILA_H
/* Pila para “acciones” de playlist o player (ej.: quitar/agregar/skip) */
typedef struct {
    int action;    /* enum opcional */
    int song_id;   /* a qué canción afectó */
    int aux;       /* dato extra */
} tAccion;

typedef struct nodoP {
    tAccion       info;
    struct nodoP* sig;
} tPilaA;

void pila_init(tPilaA** P);
int  pila_vacia(const tPilaA* P);
int  pila_push(tPilaA** P, tAccion x);
int  pila_pop(tPilaA** P, tAccion* out);
void pila_free(tPilaA** P);

#endif
