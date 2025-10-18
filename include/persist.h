#ifndef PERSIST_H
#define PERSIST_H

#include "lista.h"           /* tListaC (lista de canciones) */
#include "bst_idx.h"         /* índice por (artista,título) */
#include "trie.h"            /* trie de títulos */
#include "usuarios_lista.h"  /* lista de usuarios */

/* =========================
   B I B L I O T E C A  TXT
   =========================
   Formato de data/biblioteca.txt (una canción por línea, separador ';'):

   id;titulo;artista;genero;duracion_seg;playcount\n

   Ejemplo:
   1;Back in Black;ACDC;Rock;255;120
   2;Yellow;Coldplay;Pop;266;98
*/

int persist_biblio_load_txt(const char* path,
                            tListaC** L, tBSTI** idx, tTrie** trie);

/* Guarda un vector de canciones (p.ej. playlist) en TXT con el mismo formato. */
int persist_playlist_save_txt(const char* path, const tCancion* v, int n);


/* ======================
   U S U A R I O S  TXT
   ======================
   Formato de data/usuarios.txt (una línea por usuario, separador espacio):

   username segundos_escucha_total reproducciones_totales\n

   Ejemplo:
   lucas 1200 15
   emma  300  5
*/

int persist_usuarios_load_txt(const char* path, tListaU** L);
int persist_usuarios_save_txt(const char* path, const tListaU* L);

#endif /* PERSIST_H */
