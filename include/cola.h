#ifndef COLA_H
#define COLA_H

typedef struct {
    int song_id;   /* id cancion */
} tPlayItem;

typedef struct nodoC {
    tPlayItem     info;
    struct nodoC* sig;
} tColaN;

typedef struct {
    tColaN* pri;
    tColaN* fin;
} tColaQ;

void cola_init(tColaQ* Q);
int  cola_vacia(const tColaQ* Q);
int  cola_enq(tColaQ* Q, tPlayItem x);
int  cola_deq(tColaQ* Q, tPlayItem* out);
void cola_free(tColaQ* Q);

#endif
