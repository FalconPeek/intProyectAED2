#ifndef PERSIST_BIBLIOTECA_TXT_H
#define PERSIST_BIBLIOTECA_TXT_H

#include "lista.h"
#include "bst_idx.h"
#include "trie.h"
#include "cancion.h"

/* TXT con separador ';'
   id;titulo;artista;genero;duracion_seg;playcount
*/
int persist_biblio_load_txt(const char* path,
                            tListaC** L, tBSTI** idx, tTrie** trie);

int persist_playlist_save_txt(const char* path, const tCancion* v, int n);

#endif /* PERSIST_BIBLIOTECA_TXT_H */
