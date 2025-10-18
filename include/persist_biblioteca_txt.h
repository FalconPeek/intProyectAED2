#ifndef PERSIST_BIBLIOTECA_TXT_H
#define PERSIST_BIBLIOTECA_TXT_H

#include "lista.h"     /* tu lista de canciones tListaC */
#include "bst_idx.h"   /* índice (artista,título) */
#include "trie.h"      /* títulos por prefijo */

/* TXT "a la cátedra".
   Formato (una canción por línea, con separadores ';'):
   id;titulo;artista;genero;duracion_seg;playcount\n

   Ejemplo:
   1;Back in Black;ACDC;Rock;255;120
   2;Yellow;Coldplay;Pop;266;98
*/

/* Carga biblioteca desde TXT.
   - Inserta cada canción en la lista L.
   - Indexa en BST (clave artista,titulo).
   - Inserta título en el Trie (titulo -> id).
   Retorna 1 si ok, 0 si error (I/O o memoria). */
int persist_biblio_load_txt(const char* path,
                            tListaC** L, tBSTI** idx, tTrie** trie);

/* Guarda una playlist (vector de canciones) a TXT con mismo formato. */
int persist_playlist_save_txt(const char* path, const tCancion* v, int n);

#endif /* PERSIST_BIBLIOTECA_TXT_H */
