#include <stdio.h>
#include <string.h>
#include "app.h"

#include "lista.h"
#include "bst_idx.h"
#include "trie.h"
#include "cola.h"
#include "pila.h"
#include "grafo.h"
#include "sort.h"
#include "search.h"

#include "persist_biblioteca_txt.h"
#include "persist_usuarios_txt.h"
#include "usuarios_lista.h"
#include "usuario.h"
#include "cancion.h"

/* Rutas simples */
#define PATH_BIBLIO   "data/biblioteca.txt"
#define PATH_USUARIOS "data/usuarios.txt"
#define PATH_PLAYLIST "data/playlists/miplaylist.txt"

/* Estado global del app_run (variables locales al archivo) */
static tListaC* g_biblio = NULL;
static tBSTI*   g_idx    = NULL;
static tTrie*   g_trie   = NULL;

static tColaQ   g_queue;
static tPilaA*  g_undo  = NULL;
static tGrafo   g_G;        /* recomendaciones */
static tListaU* g_users = NULL;
static tUsuario* g_user = NULL; /* usuario logueado */
static tListaC* g_playlist = NULL;

/* --- Helpers de UI --- */
static void banner(void){
    puts("==============================================");
    puts("   Reproductor & Recomendador Offline (AED2)  ");
    puts("==============================================");
}
static void menu(void){
    puts("\nMENU");
    puts("1) Cargar biblioteca (TXT)");
    puts("2) Login usuario");
    puts("3) Buscar cancion (BST por artista,titulo)");
    puts("4) Autocompletar por prefijo (Trie)");
    puts("5) Playlist: agregar/quitar/mostrar");
    puts("6) Reproducir: encolar / siguiente / ver cola");
    puts("7) Undo (deshacer ultima accion de playlist)");
    puts("8) Construir grafo y recomendar (BFS)");
    puts("9) Top-N mas reproducidas (global)");
    puts("10) Guardar playlist y usuarios");
    puts("0) Salir");
}

/* Construye grafo: conecta canciones con mismo artista */
static void construir_grafo_por_artista(void){
    /* necesitamos conocer max id para dimensionar G */
    int maxId = -1;
    for(tListaC* it=g_biblio; it; it=it->sig)
        if(it->info.id > maxId) maxId = it->info.id;
    if(maxId < 0) { grafo_init(&g_G, 0); return; }
    grafo_init(&g_G, maxId+1);

    /* por artista: O(n^2) simple para MVP */
    for(tListaC* a=g_biblio; a; a=a->sig){
        for(tListaC* b=a->sig; b; b=b->sig){
            if(strcmp(a->info.artista, b->info.artista)==0){
                grafo_add_edge(&g_G, a->info.id, b->info.id);
            }
        }
    }
    puts("OK: Grafo construido por artista.");
}

/* Mostrar una lista */
static void mostrar_lista(const tListaC* L, const char* titulo){
    printf("---- %s ----\n", titulo);
    for(; L; L=L->sig) cancion_print(&L->info);
}

/* Concatena biblioteca a vector (para Top-N) */
static int biblio_to_array(tCancion** out){
    int n = lista_len(g_biblio);
    if(n<=0){ *out=NULL; return 0; }
    tCancion* v = (tCancion*)malloc(sizeof(tCancion)*n);
    if(!v){ *out=NULL; return 0; }
    lista_to_array(g_biblio, v, n);
    *out = v; return n;
}

/* --- Menús --- */
static void do_cargar_biblio(void){
    if(!g_trie){ trie_init(&g_trie); }
    if(persist_biblio_load_txt(PATH_BIBLIO, &g_biblio, &g_idx, &g_trie))
        puts("OK: Biblioteca cargada.");
    else
        puts("ERROR: No se pudo cargar biblioteca.");
}

static void do_login(void){
    char user[50];
    printf("Usuario: ");
    if(scanf(" %49s", user)!=1) return;

    if(!g_users) listaU_init(&g_users);
    tUsuario* u = listaU_find_username(g_users, user);
    if(!u){
        tUsuario nu; memset(&nu,0,sizeof(nu));
        strncpy(nu.username, user, sizeof(nu.username)-1);
        if(!listaU_push_front(&g_users, &nu)){ puts("Sin memoria."); return; }
        u = listaU_find_username(g_users, user);
        puts("Usuario creado.");
    }else{
        puts("Usuario encontrado.");
    }
    g_user = u;
    usuario_print(g_user);

    /* Cargar usuarios desde disco si no lo hicimos aún */
    static int loaded = 0;
    if(!loaded){
        if(persist_usuarios_load_txt(PATH_USUARIOS, &g_users))
            puts("Usuarios (TXT): cargados/creados.");
        else
            puts("Usuarios (TXT): ERROR de carga.");
        loaded = 1;
    }
}

static void do_buscar_bst(void){
    char artista[50], titulo[50];
    printf("Artista: "); if(scanf(" %49[^\n]", artista)!=1) return;
    printf("Titulo:  "); if(scanf(" %49[^\n]", titulo)!=1) return;
    tCancion* c = bst_find(g_idx, artista, titulo);
    if(!c) puts("No encontrada.");
    else   cancion_print(c);
}

static void do_buscar_trie(void){
    char pref[50];
    printf("Prefijo de titulo: "); if(scanf(" %49[^\n]", pref)!=1) return;
    int ids[10]; int k = trie_collect_prefix(g_trie, pref, ids, 10);
    if(k<=0){ puts("Sin coincidencias."); return; }
    for(int i=0;i<k;i++){
        tCancion* c = lista_find_by_id(g_biblio, ids[i]);
        if(c) cancion_print(c);
    }
}

static void do_playlist(void){
    int op=-1;
    do{
        puts("\nPLAYLIST");
        puts("1) Agregar por id");
        puts("2) Quitar por id");
        puts("3) Mostrar playlist");
        puts("0) Volver");
        printf("> "); if(scanf("%d",&op)!=1) return;
        if(op==1){
            int id; printf("id: "); if(scanf("%d",&id)!=1) continue;
            tCancion* c = lista_find_by_id(g_biblio, id);
            if(!c){ puts("No existe en biblioteca."); continue; }
            if(!lista_push_front(&g_playlist, *c)){ puts("Sin memoria."); continue; }
            tAccion a = { .action=1, .song_id=id, .aux=0 }; /* ADD */
            pila_push(&g_undo, a);
            puts("Agregada.");
        }else if(op==2){
            int id; printf("id: "); if(scanf("%d",&id)!=1) continue;
            if(lista_remove_by_id(&g_playlist, id)){
                tAccion a = { .action=2, .song_id=id, .aux=0 }; /* DEL */
                pila_push(&g_undo, a);
                puts("Quitada.");
            }else puts("No estaba en playlist.");
        }else if(op==3){
            mostrar_lista(g_playlist, "Playlist actual");
        }
    }while(op!=0);
}

static void do_undo(void){
    tAccion a;
    if(!pila_pop(&g_undo, &a)){ puts("Nada para deshacer."); return; }
    if(a.action==1){ /* se había agregado -> ahora quitar */
        if(lista_remove_by_id(&g_playlist, a.song_id)) puts("Undo: se quitó.");
        else puts("Undo: no estaba.");
    }else if(a.action==2){ /* se había quitado -> reinsertar */
        tCancion* c = lista_find_by_id(g_biblio, a.song_id);
        if(c && lista_push_front(&g_playlist, *c)) puts("Undo: se reinsertó.");
        else puts("Undo: fallo al reinsertar.");
    }else{
        puts("Accion desconocida.");
    }
}

static void do_repro(void){
    int op=-1;
    do{
        puts("\nREPRODUCIR");
        puts("1) Encolar por id");
        puts("2) Siguiente (play)");
        puts("3) Ver cola (ids)");
        puts("0) Volver");
        printf("> "); if(scanf("%d",&op)!=1) return;
        if(op==1){
            int id; printf("id: "); if(scanf("%d",&id)!=1) continue;
            tCancion* c = lista_find_by_id(g_biblio, id);
            if(!c){ puts("No existe en biblioteca."); continue; }
            tPlayItem it = { .song_id=id };
            if(cola_enq(&g_queue, it)) puts("Encolada.");
            else puts("Sin memoria.");
        }else if(op==2){
            tPlayItem it;
            if(!cola_deq(&g_queue, &it)){ puts("Cola vacia."); continue; }
            tCancion* c = lista_find_by_id(g_biblio, it.song_id);
            if(!c){ puts("Cancion inexistente (id huérfano)."); continue; }
            c->playcount++;
            if(g_user){
                g_user->reproducciones_totales++;
                g_user->segundos_escucha_total += c->duracion_seg;
            }
            printf("▶ Reproduciendo: "); cancion_print(c);
            if(g_user){ printf("Usuario "); usuario_print(g_user); }
        }else if(op==3){
            /* volcar cola sin destruir: hacemos pop y volvemos a encolar */
            tColaQ aux; cola_init(&aux);
            tPlayItem it;
            puts("Cola (frente→fondo):");
            while(cola_deq(&g_queue, &it)){
                printf("%d ", it.song_id);
                cola_enq(&aux, it);
            }
            puts("");
            /* restaurar */
            while(cola_deq(&aux, &it)) cola_enq(&g_queue, it);
        }
    }while(op!=0);
}

static void do_recomendar(void){
    construir_grafo_por_artista();
    int id,k; printf("Desde id: "); if(scanf("%d",&id)!=1) return;
    printf("Cuantos? "); if(scanf("%d",&k)!=1) return;
    int* out = (int*)malloc(sizeof(int)*k);
    if(!out){ puts("Sin memoria."); return; }
    int n = grafo_bfs(&g_G, id, out, k);
    if(n<=0) puts("No hay recomendaciones.");
    else{
        puts("Recomendadas:");
        for(int i=0;i<n;i++){
            tCancion* c = lista_find_by_id(g_biblio, out[i]);
            if(c) cancion_print(c);
        }
    }
    free(out);
}

static void do_top(void){
    tCancion* v=NULL; int n=biblio_to_array(&v);
    if(n<=0){ puts("Biblio vacia."); return; }
    long comps=0, swaps=0;
    sort_por_playcount_insercion(v,n,&comps,&swaps);
    int N=5; if(N>n) N=n;
    puts("Top-N (playcount desc):");
    for(int i=0;i<N;i++) cancion_print(&v[i]);
    printf("Métricas inserción: comps=%ld swaps=%ld\n", comps, swaps);
    free(v);
}

static void do_guardar(void){
    /* Guardar playlist */
    int n = lista_len(g_playlist);
    if(n>0){
        tCancion* v = (tCancion*)malloc(sizeof(tCancion)*n);
        if(!v){ puts("Sin memoria."); goto usuarios; }
        lista_to_array(g_playlist, v, n);
        if(persist_playlist_save_txt(PATH_PLAYLIST, v, n))
            puts("OK: Playlist guardada.");
        else
            puts("ERROR: al guardar playlist.");
        free(v);
    }else puts("Playlist vacia: nada que guardar.");

usuarios:
    if(persist_usuarios_save_txt(PATH_USUARIOS, g_users))
        puts("OK: Usuarios guardados.");
    else
        puts("ERROR: al guardar usuarios.");
}

/* --- Loop principal --- */
void app_run(void){
    banner();
    cola_init(&g_queue);
    pila_init(&g_undo);
    trie_init(&g_trie); /* por si cargamos biblio luego */

    int op=-1;
    do{
        menu();
        printf("> ");
        if(scanf("%d",&op)!=1){ puts("Entrada invalida."); return; }

        switch(op){
            case 1: do_cargar_biblio(); break;
            case 2: do_login(); break;
            case 3: do_buscar_bst(); break;
            case 4: do_buscar_trie(); break;
            case 5: do_playlist(); break;
            case 6: do_repro(); break;
            case 7: do_undo(); break;
            case 8: do_recomendar(); break;
            case 9: do_top(); break;
            case 10: do_guardar(); break;
            case 0:  break;
            default: puts("Opcion invalida.");
        }
    }while(op!=0);

    /* liberar memoria */
    lista_free(&g_playlist);
    lista_free(&g_biblio);
    bst_free(&g_idx);
    trie_free(&g_trie);
    cola_free(&g_queue);
    pila_free(&g_undo);
    listaU_free(&g_users);
}
