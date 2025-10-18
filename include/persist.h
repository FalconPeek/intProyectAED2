#ifndef PERSIST_H
#define PERSIST_H
#include "lista.h"
#include "bst_idx.h"
#include "trie.h"

/* Cargar biblioteca en lista + índice + trie.
   CSV esperado: id;titulo;artista;genero;duracion_seg;playcount */
int persist_load_biblioteca_csv(const char* path_csv,
                                tListaC** L, tBSTI** idx, tTrie** trie);

/* Guardar playlist como CSV: id;titulo;artista;... (tomado desde array) */
int persist_save_playlist_csv(const char* path_csv, const tCancion* v, int n);

#endif
