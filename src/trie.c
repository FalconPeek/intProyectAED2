/* Trie de títulos (a-z y espacio) — src/trie.c */
#include "trie.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* Mapea char -> índice [0..26]: a..z => 0..25, espacio => 26. Otros -> 26 */
static int chi(int c){
    if(c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
    if(c >= 'a' && c <= 'z') return c - 'a';
    if(c == ' ') return 26;
    return 26; /* simplificación */
}

void trie_init(tTrie** T){
    if(*T) return;
    *T = (tTrie*)calloc(1, sizeof(tTrie));
    if(*T) (*T)->song_id = -1;
}

static void trie_free_rec(tTrie* T){
    if(!T) return;
    for(int i=0;i<TRIE_SIGMA;i++) trie_free_rec(T->next[i]);
    free(T);
}

void trie_free(tTrie** T){
    if(!*T) return;
    trie_free_rec(*T);
    *T = NULL;
}

int trie_insert(tTrie* T, const char* titulo, int song_id){
    if(!T || !titulo) return 0;
    tTrie* cur = T;
    for(const char* p=titulo; *p; ++p){
        int idx = chi((unsigned char)*p);
        if(!cur->next[idx]){
            cur->next[idx] = (tTrie*)calloc(1, sizeof(tTrie));
            if(!cur->next[idx]) return 0;
            cur->next[idx]->song_id = -1;
        }
        cur = cur->next[idx];
    }
    cur->song_id = song_id; /* marca fin de palabra */
    return 1;
}

bool trie_search(const tTrie* T, const char* titulo, int* song_id_out){
    const tTrie* cur = T;
    if(!cur || !titulo) return false;
    for(const char* p=titulo; *p; ++p){
        int idx = chi((unsigned char)*p);
        cur = cur->next[idx];
        if(!cur) return false;
    }
    if(cur->song_id == -1) return false;
    if(song_id_out) *song_id_out = cur->song_id;
    return true;
}

/* DFS limitado para recolectar hasta k IDs a partir de un nodo */
static int collect_dfs(const tTrie* T, int* out_ids, int k, int* count){
    if(!T || *count >= k) return 1;
    if(T->song_id != -1){
        out_ids[(*count)++] = T->song_id;
        if(*count >= k) return 1;
    }
    for(int i=0;i<TRIE_SIGMA;i++){
        if(T->next[i]){
            if(!collect_dfs(T->next[i], out_ids, k, count)) return 0;
            if(*count >= k) break;
        }
    }
    return 1;
}

int trie_collect_prefix(const tTrie* T, const char* pref, int* out_ids, int k){
    const tTrie* cur = T;
    if(!cur || !pref || k<=0) return 0;
    for(const char* p=pref; *p; ++p){
        int idx = chi((unsigned char)*p);
        cur = cur->next[idx];
        if(!cur) return 0;
    }
    int count = 0;
    collect_dfs(cur, out_ids, k, &count);
    return count;
}
