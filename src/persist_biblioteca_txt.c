#include "persist_biblioteca_txt.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

/* NOTA IMPORTANTE (lectura en texto):
   Usamos "lectura adelantada" al estilo de cátedra:
   1) fscanf de la primera línea
   2) while (!feof) { procesar si r==6; volver a fscanf }
   Esto evita procesar EOF como dato válido. */

int persist_biblio_load_txt(const char* path,
                            tListaC** L, tBSTI** idx, tTrie** trie)
{
    FILE* f = fopen(path, "r");
    if(!f){
        /* Si no existe, lo consideramos "ok, vacío". */
        return (errno == ENOENT) ? 1 : 0;
    }

    tCancion c;
    /* buffers temporales para strings con separador ';' */
    /* Usamos especificadores %49[^;] para leer hasta ';' sin comer espacios. */
    int r = fscanf(f, " %d ; %49[^;] ; %49[^;] ; %49[^;] ; %d ; %d",
                   &c.id, c.titulo, c.artista, c.genero, &c.duracion_seg, &c.playcount);

    while(!feof(f)){
        if(r == 6){
            /* Inserciones en estructuras */
            if(!lista_push_front(L, c)){
                fclose(f); return 0;
            }
            if(!bst_insert(idx, &c)){ /* si duplicado, ignoramos índice pero seguimos */
                /* no abortamos: el índice es auxiliar. */
            }
            /* insertar título en trie (normalizar título si tu trie lo requiere) */
            trie_insert(*trie, c.titulo, c.id);
        }
        r = fscanf(f, " %d ; %49[^;] ; %49[^;] ; %49[^;] ; %d ; %d",
                   &c.id, c.titulo, c.artista, c.genero, &c.duracion_seg, &c.playcount);
    }

    fclose(f);
    return 1;
}

int persist_biblio_save_txt(const char* path, const tListaC* L){
    FILE* f = fopen(path, "w");
    if(!f) return 0;

    for(const tListaC* it = L; it; it = it->sig){
        if(fprintf(f, "%d;%s;%s;%s;%d;%d\n",
                   it->info.id,
                   it->info.titulo,
                   it->info.artista,
                   it->info.genero,
                   it->info.duracion_seg,
                   it->info.playcount) <= 0){
            fclose(f);
            return 0;
        }
    }

    fclose(f);
    return 1;
}

int persist_playlist_save_txt(const char* path, const tCancion* v, int n){
    FILE* f = fopen(path, "w");
    if(!f) return 0;

    for(int i=0; i<n; ++i){
        int m = fprintf(f, "%d;%s;%s;%s;%d;%d\n",
                        v[i].id, v[i].titulo, v[i].artista, v[i].genero,
                        v[i].duracion_seg, v[i].playcount);
        if(m <= 0){ fclose(f); return 0; }
    }

    fclose(f);
    return 1;
}

int persist_playlist_load_txt(const char* path, tListaC** playlist){
    FILE* f = fopen(path, "r");
    if(!f){
        return (errno == ENOENT) ? 1 : 0;
    }

    tCancion* arr = NULL;
    int n = 0, cap = 0;

    tCancion c;
    int r = fscanf(f, " %d ; %49[^;] ; %49[^;] ; %49[^;] ; %d ; %d",
                   &c.id, c.titulo, c.artista, c.genero, &c.duracion_seg, &c.playcount);

    while(!feof(f)){
        if(r == 6){
            if(n >= cap){
                int ncap = (cap == 0) ? 8 : cap * 2;
                tCancion* tmp = (tCancion*)realloc(arr, sizeof(tCancion) * ncap);
                if(!tmp){ free(arr); fclose(f); return 0; }
                arr = tmp; cap = ncap;
            }
            arr[n++] = c;
        }
        r = fscanf(f, " %d ; %49[^;] ; %49[^;] ; %49[^;] ; %d ; %d",
                   &c.id, c.titulo, c.artista, c.genero, &c.duracion_seg, &c.playcount);
    }

    fclose(f);

    for(int i = n - 1; i >= 0; --i){
        if(!lista_push_front(playlist, arr[i])){ free(arr); return 0; }
    }

    free(arr);
    return 1;
}
