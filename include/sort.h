#ifndef SORT_H
#define SORT_H
#include "cancion.h"

/* Ordenar arrays de tCancion por distintos criterios */
void sort_por_titulo_burbuja(tCancion v[], int n, long* comps, long* swaps);
void sort_por_playcount_insercion(tCancion v[], int n, long* comps, long* swaps);

#endif
