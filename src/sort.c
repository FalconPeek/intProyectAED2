/* Ordenamientos simples con métricas — src/sort.c */
#include "sort.h"

void sort_por_titulo_burbuja(tCancion v[], int n, long* comps, long* swaps){
    if(comps) *comps = 0;
    if(swaps) *swaps = 0;
    for(int i=0;i<n-1;i++){
        for(int j=0;j<n-1-i;j++){
            if(comps) (*comps)++;
            if(cancion_cmp_titulo(&v[j], &v[j+1]) > 0){
                tCancion t = v[j]; v[j] = v[j+1]; v[j+1] = t;
                if(swaps) (*swaps)++;
            }
        }
    }
}

void sort_por_playcount_insercion(tCancion v[], int n, long* comps, long* swaps){
    if(comps) *comps = 0;
    if(swaps) *swaps = 0;
    for(int i=1;i<n;i++){
        tCancion key = v[i];
        int j = i-1;
        while(j>=0){
            if(comps) (*comps)++;
            /* Descendente por playcount (más reproducidas primero) */
            if(v[j].playcount >= key.playcount) break;
            v[j+1] = v[j];
            if(swaps) (*swaps)++;
            j--;
        }
        v[j+1] = key;
        if(swaps && j+1 != i) (*swaps)++;
    }
}
