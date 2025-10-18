#ifndef SEARCH_H
#define SEARCH_H
#include "cancion.h"

/* Búsqueda secuencial por id (o título) */
int search_lineal_por_id(const tCancion v[], int n, int id, long* comps);

/* Binaria: requiere vector ordenado por título */
int search_binaria_por_titulo(const tCancion v[], int n, const char* titulo, long* comps);

#endif
