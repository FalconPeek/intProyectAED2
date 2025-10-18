#ifndef CANCION_H
#define CANCION_H
#include "commons.h"

/* Identificador único opcional (secuencial). */
typedef struct {
    int     id;
    tString titulo;
    tString artista;
    tString genero;
    int     duracion_seg;
    int     playcount;   /* global */
} tCancion;

/* Helpers de impresión/parsing (opcionales, pero útiles) */
void cancion_print(const tCancion* c);
int  cancion_cmp_titulo(const tCancion* a, const tCancion* b);           /* strcmp por título */
int  cancion_cmp_artista_titulo(const tCancion* a, const tCancion* b);  /* (artista,titulo) */

#endif
