/* Grafo de recomendaciones (matriz 0/1) — src/grafo.c */
#include "grafo.h"
#include <string.h>

void grafo_init(tGrafo* G, int n){
    if(n < 0) n = 0;
    if(n > GMAX) n = GMAX;
    G->n = n;
    memset(G->adj, 0, sizeof(G->adj));
}

void grafo_add_edge(tGrafo* G, int u, int v){
    if(u<0 || v<0 || u>=G->n || v>=G->n) return;
    if(u == v) return;
    G->adj[u][v] = 1;
    G->adj[v][u] = 1; /* no dirigido */
}

int  grafo_are_adj(const tGrafo* G, int u, int v){
    if(u<0 || v<0 || u>=G->n || v>=G->n) return 0;
    return G->adj[u][v] != 0;
}

int  grafo_bfs(const tGrafo* G, int src, int* out_ids, int k){
    if(!G || src<0 || src>=G->n || k<=0) return 0;
    unsigned char vis[GMAX]; memset(vis, 0, sizeof(vis));
    int Q[GMAX]; int qh=0, qt=0;

    Q[qt++] = src; vis[src] = 1;
    int count = 0;

    while(qh < qt && count < k){
        int u = Q[qh++];
        for(int v=0; v<G->n; ++v){
            if(G->adj[u][v] && !vis[v]){
                vis[v] = 1;
                Q[qt++] = v;
                if(v != src){                /* no devolver el mismo */
                    out_ids[count++] = v;
                    if(count >= k) break;
                }
            }
        }
    }
    return count;
}
