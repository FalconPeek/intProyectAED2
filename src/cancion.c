#include "cancion.h"
#include <stdio.h>
#include <string.h>

void cancion_print(const tCancion* c){
    if(!c){ puts("(cancion NULL)"); return; }
    printf("#%d %-30s | %-20s | %-10s | %4ds | plays=%d\n",
           c->id, c->titulo, c->artista, c->genero, c->duracion_seg, c->playcount);
}

int cancion_cmp_titulo(const tCancion* a, const tCancion* b){
    return strcmp(a->titulo, b->titulo);
}

int cancion_cmp_artista_titulo(const tCancion* a, const tCancion* b){
    int c = strcmp(a->artista, b->artista);
    if(c != 0) return c;
    return strcmp(a->titulo, b->titulo);
}
