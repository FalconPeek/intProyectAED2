#ifndef GRAFO_H
#define GRAFO_H

/* Grafo no dirigido (o dirigido) simple.
   Nodos = canciones o artistas (vos elegís).
   Para empezar: matriz de adyacencia 0/1.
*/
#define GMAX 1024

typedef struct {
    int n;                 /* #nodos */
    unsigned char adj[GMAX][GMAX]; /* 0/1 */
} tGrafo;

void grafo_init(tGrafo* G, int n);
void grafo_add_edge(tGrafo* G, int u, int v);
int  grafo_are_adj(const tGrafo* G, int u, int v);

/* BFS para recomendar desde un nodo origen: llena out_ids con vecinos/alcanzables */
int  grafo_bfs(const tGrafo* G, int src, int* out_ids, int k);

#endif
