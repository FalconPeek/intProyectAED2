/* Búsquedas — src/search.c */
#include "search.h"
#include <string.h>

int search_lineal_por_id(const tCancion v[], int n, int id, long* comps){
    if(comps) *comps = 0;
    for(int i=0;i<n;i++){
        if(comps) (*comps)++;
        if(v[i].id == id) return i;
    }
    return -1;
}

/* Requiere vector ordenado por título (ASC) */
int search_binaria_por_titulo(const tCancion v[], int n, const char* titulo, long* comps){
    if(comps) *comps = 0;
    int lo = 0, hi = n-1;
    while(lo <= hi){
        int mid = lo + (hi - lo)/2;
        if(comps) (*comps)++;
        int c = strcmp(v[mid].titulo, titulo);
        if(c == 0) return mid;
        if(c < 0) lo = mid + 1;
        else      hi = mid - 1;
    }
    return -1;
}
