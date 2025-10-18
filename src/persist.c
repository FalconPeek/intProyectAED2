/* Persistencia TXT "a la cátedra"
 * - Biblioteca: data/biblioteca.txt con separador ';'
 * - Playlist:   guarda vector de canciones con el mismo formato
 * - Usuarios:   data/usuarios.txt (campos separados por espacio)
 *
 * Uso de I/O en texto:
 *   - fopen("r"/"w")
 *   - fscanf / fprintf
 *   - lectura adelantada: primer fscanf y luego while(!feof(f)) { ...; fscanf ... }
 */

#include "persist.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* =========================
 *  B I B L I O T E C A
 * =========================
 */
int persist_biblio_load_txt(const char* path,
                            tListaC** L, tBSTI** idx, tTrie** trie)
{
    FILE* f = fopen(path, "r");
    if(!f){
        /* Si el archivo no existe, consideramos "colección vacía" (OK). */
        return (errno == ENOENT) ? 1 : 0;
    }

    tCancion c;
    /* lectura adelantada
       - %49[^;] lee hasta ';' permitiendo espacios
       - espacios opcionales alrededor de ';' con " ; "
    */
    int r = fscanf(f, " %d ; %49[^;] ; %49[^;] ; %49[^;] ; %d ; %d",
                   &c.id, c.titulo, c.artista, c.genero, &c.duracion_seg, &c.playcount);

    while(!feof(f)){
        if(r == 6){
            /* insertar a lista principal */
            if(!lista_push_front(L, c)){
                fclose(f);
                return 0; /* sin memoria */
            }
            /* indexar en ABB por (artista,título); si duplicado, lo ignoramos */
            if(!bst_insert(idx, &c)){
                /* índice auxiliar: no abortamos por duplicado */
            }
            /* insertar en trie: título -> id */
            trie_insert(*trie, c.titulo, c.id);
        }
        /* volver a leer para siguiente iteración */
        r = fscanf(f, " %d ; %49[^;] ; %49[^;] ; %49[^;] ; %d ; %d",
                   &c.id, c.titulo, c.artista, c.genero, &c.duracion_seg, &c.playcount);
    }

    fclose(f);
    return 1;
}

int persist_playlist_save_txt(const char* path, const tCancion* v, int n){
    FILE* f = fopen(path, "w");
    if(!f) return 0;

    for(int i=0; i<n; ++i){
        /* mismo formato que biblioteca.txt */
        if(fprintf(f, "%d;%s;%s;%s;%d;%d\n",
                   v[i].id, v[i].titulo, v[i].artista, v[i].genero,
                   v[i].duracion_seg, v[i].playcount) <= 0){
            fclose(f);
            return 0;
        }
    }

    fclose(f);
    return 1;
}

/* ===================
 *  U S U A R I O S
 * ===================
 */
int persist_usuarios_load_txt(const char* path, tListaU** L){
    FILE* f = fopen(path, "r");
    if(!f){
        /* Ausente => colección vacía (OK) */
        return (errno == ENOENT) ? 1 : 0;
    }

    tUsuario u;
    /* lectura adelantada:
       - %49s para username sin espacios (si querés permitir espacios, cambia a %[^\n] y define otro separador)
       - %ld para segundos (long), %d para reproducciones (int)
    */
    int r = fscanf(f, " %49s %ld %d",
                   u.username, &u.segundos_escucha_total, &u.reproducciones_totales);

    while(!feof(f)){
        if(r == 3){
            if(!listaU_push_front(L, &u)){
                fclose(f);
                return 0; /* sin memoria */
            }
        }
        r = fscanf(f, " %49s %ld %d",
                   u.username, &u.segundos_escucha_total, &u.reproducciones_totales);
    }

    fclose(f);
    return 1;
}

int persist_usuarios_save_txt(const char* path, const tListaU* L){
    FILE* f = fopen(path, "w");
    if(!f) return 0;

    for(; L; L = L->sig){
        /* username sin espacios */
        if(fprintf(f, "%s %ld %d\n",
                   L->info.username,
                   L->info.segundos_escucha_total,
                   L->info.reproducciones_totales) <= 0){
            fclose(f);
            return 0;
        }
    }

    fclose(f);
    return 1;
}
