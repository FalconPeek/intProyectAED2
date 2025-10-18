#ifndef TRIE_H
#define TRIE_H
#include <stdbool.h>

#define TRIE_SIGMA 27  /* a-z + espacio (simplificado) */
typedef struct trieN {
    int  song_id;                /* -1 si no termina palabra */
    struct trieN* next[TRIE_SIGMA];
} tTrie;

void trie_init(tTrie** T);               /* T=NULL -> crear raíz */
void trie_free(tTrie** T);
int  trie_insert(tTrie* T, const char* titulo, int song_id); /* 1 ok */
bool trie_search(const tTrie* T, const char* titulo, int* song_id_out);
/* recorrido por prefijo: llenar hasta k IDs que empiecen con prefijo */
int  trie_collect_prefix(const tTrie* T, const char* pref, int* out_ids, int k);

#endif
