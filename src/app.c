#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
/* ===== TUI ANSI sin dependencias ===== */
#include <termios.h>
#include <unistd.h>

static void ui_flush_input(void){
    int c;
    while((c=getchar())!='\n' && c!=EOF) { /* vacía buffer */ }
}


static void ansi_clear(void){ printf("\x1b[2J\x1b[H"); }                 /* limpia */
static void ansi_hide_cursor(void){ printf("\x1b[?25l"); }
static void ansi_show_cursor(void){ printf("\x1b[?25h"); }
static void ansi_color(int fg){ printf("\x1b[%dm", fg); }               /* 30..37 */
static void ansi_bold(int on){ printf("\x1b[%dm", on?1:22); }
static void ansi_reset(void){ printf("\x1b[0m"); }

/* lectura cruda de teclas (sin Enter) */
static int tty_raw_on(struct termios* orig){
    struct termios raw;
    if(tcgetattr(STDIN_FILENO, orig) < 0) return 0;
    raw = *orig;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;
    return tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0;
}
static void tty_raw_off(const struct termios* orig){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, orig);
}

/* lee key: retorna códigos: 13=Enter, 27/’[’ flechas translate a 1000/1001 */
enum { KEY_ENTER=13, KEY_UP=1000, KEY_DOWN=1001 };
static int read_key(void){
    int c = getchar();
    if(c == 13 || c == '\n') return KEY_ENTER;
    if(c == 27){ /* ESC seq */
        int c1 = getchar();
        if(c1 == '['){
            int c2 = getchar();
            if(c2 == 'A') return KEY_UP;
            if(c2 == 'B') return KEY_DOWN;
        }
    }
    return c;
}

/* ===== Helpers de UI (pantalla limpia y headers) ===== */
static void ui_clear(void){ printf("\x1b[2J\x1b[H"); }  /* limpia y lleva cursor al (1,1) */

static void ui_header(const char* title){
    ui_clear();
    ansi_bold(1); ansi_color(36);
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║  %s\n", title);
    printf("╚════════════════════════════════════════════════════════╝\n");
    ansi_reset();
    puts("");
}

/* Espera Enter para continuar (no deja basura en pantalla) */
static void ui_press_enter(void){
    puts("\n[Enter] para continuar...");
    struct termios orig;
    if(tty_raw_on(&orig)){
        for(;;){
            int k = read_key();
            if(k == KEY_ENTER) break;
        }
        tty_raw_off(&orig);
    }else{
        /* fallback */
        int c; do{ c=getchar(); }while(c!='\n' && c!=EOF);
    }
}
/* ===== App principal ===== */


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

/* Elige una canción usando prefijo de título (Trie) y devuelve el song_id,
   o -1 si no hubo selección válida. Muestra hasta K coincidencias. */
static int elegir_cancion_por_prefijo(void){
    if(!g_trie){ puts("Trie no inicializado. Cargá la biblioteca primero."); return -1; }

    char pref[50];
    printf("Prefijo de titulo: ");
    ui_flush_input();
    if(scanf(" %49[^\n]", pref) != 1) return -1;

    enum { KMAX = 10 };
    int ids[KMAX];
    int k = trie_collect_prefix(g_trie, pref, ids, KMAX);
    if(k <= 0){
        puts("Sin coincidencias para ese prefijo.");
        return -1;
    }

    puts("Coincidencias:");
    int printed = 0;
    for(int i = 0; i < k; ++i){
        tCancion* c = lista_find_by_id(g_biblio, ids[i]);
        if(c){
            printf("%2d) ", printed+1);
            cancion_print(c);
            ids[printed] = ids[i]; /* compactar por si alguna id no existiera */
            printed++;
        }
    }
    if(printed == 0){
        puts("No se encontraron canciones válidas en biblioteca.");
        return -1;
    }

    int sel = 0;
    printf("Elegí una opción (1-%d, 0 para cancelar): ", printed);
    ui_flush_input();
    if(scanf("%d", &sel) != 1 || sel < 1 || sel > printed) {
        puts("Cancelado o selección inválida.");
        return -1;
    }
    return ids[sel-1];
}

/* Elige una canción por prefijo (Trie) pero SOLO entre las que están en la playlist.
   Devuelve song_id o -1 si no hubo selección válida. */
static int elegir_cancion_por_prefijo_en_playlist(void){
    if(!g_trie){ puts("Trie no inicializado. Cargá la biblioteca primero."); return -1; }

    char pref[50];
    printf("Prefijo de titulo (en playlist): ");
    ui_flush_input();
    if(scanf(" %49[^\n]", pref) != 1) return -1;

    enum { KMAX = 32 };     /* más amplio, por si la playlist es grande */
    int ids_tmp[KMAX];
    int k = trie_collect_prefix(g_trie, pref, ids_tmp, KMAX);
    if(k <= 0){
        puts("Sin coincidencias para ese prefijo.");
        return -1;
    }

    /* Filtrar: solo mostrar los que están en la playlist */
    int ids[KMAX];
    int printed = 0;
    puts("Coincidencias en playlist:");
    for(int i=0;i<k;i++){
        tCancion* cLib = lista_find_by_id(g_biblio, ids_tmp[i]);
        tCancion* cPl  = lista_find_by_id(g_playlist, ids_tmp[i]);
        if(cLib && cPl){
            printf("%2d) ", printed+1);
            cancion_print(cLib);
            ids[printed++] = ids_tmp[i];
        }
    }
    if(printed == 0){
        puts("No hay canciones de esa búsqueda dentro de tu playlist.");
        return -1;
    }

    int sel = 0;
    printf("Elegí una opción (1-%d, 0 para cancelar): ", printed);
    ui_flush_input();
    if(scanf("%d",&sel)!=1 || sel<1 || sel>printed){
        puts("Cancelado o selección inválida.");
        return -1;
    }
    return ids[sel-1];
}

static const char* g_menu_items[] = {
    "Cargar biblioteca (TXT)",
    "Login usuario",
    "Buscar cancion (BST por artista,titulo)",
    "Autocompletar por prefijo (Trie)",
    "Playlist: agregar/quitar/mostrar",
    "Reproducir: encolar / siguiente / ver cola",
    "Undo (deshacer ultima accion de playlist)",
    "Construir grafo y recomendar (BFS)",
    "Top-N mas reproducidas (global)",
    "Guardar playlist y usuarios",
    "Salir"
};
static const int g_menu_count = 11;

static int tui_menu_select(void){
    struct termios orig;
    if(!tty_raw_on(&orig)) return -1;
    ansi_hide_cursor();

    int sel = 0, running = 1;
    while(running){
        ansi_clear();
        /* marco y título */
        ansi_bold(1); ansi_color(36);
        puts("╔════════════════════════════════════════════════════════╗");
        puts("║    Reproductor & Recomendador Offline (AED2 - TUI)     ║");
        puts("╚════════════════════════════════════════════════════════╝");
        ansi_reset();

        puts("");
        for(int i=0;i<g_menu_count;i++){
            if(i==sel){
                ansi_bold(1); ansi_color(32);
                printf(" -> %2d) %s\n", i+1, g_menu_items[i]);
                ansi_reset();
            }else{
                printf("   %2d) %s\n", i+1, g_menu_items[i]);
            }
        }
        puts("\nUsa ↑/↓ y Enter. ESC para salir.");

        int k = read_key();
        if(k == KEY_UP){ if(--sel < 0) sel = g_menu_count-1; }
        else if(k == KEY_DOWN){ if(++sel >= g_menu_count) sel = 0; }
        else if(k == KEY_ENTER){ running = 0; }
        else if(k == 27){ sel = g_menu_count-1; running = 0; } /* ESC = Salir */
    }

    ansi_show_cursor();
    tty_raw_off(&orig);
    return sel+1; /* opciones 1..11 (igual que tu switch) */
}


/* --- Menús --- */
static void do_cargar_biblio(void){
    ui_header("Cargar biblioteca (TXT)");
    if(!g_trie) trie_init(&g_trie);

    if(persist_biblio_load_txt(PATH_BIBLIO, &g_biblio, &g_idx, &g_trie))
        puts("OK: Biblioteca cargada.");
    else
        puts("ERROR: No se pudo cargar biblioteca. Verifique data/biblioteca.txt");

    ui_press_enter();
}


static void do_login(void){
    ui_header("Login de usuario");
    char user[50];
    printf("Usuario: ");
    ui_flush_input();
    if(scanf(" %49s", user)!=1){ puts("Entrada invalida."); ui_press_enter(); return; }

    if(!g_users) listaU_init(&g_users);
    static int loaded = 0;
    if(!loaded){
        if(persist_usuarios_load_txt(PATH_USUARIOS, &g_users))
            puts("Usuarios (TXT): cargados/creados.");
        else
            puts("Usuarios (TXT): ERROR de carga.");
        loaded = 1;
    }

    tUsuario* u = listaU_find_username(g_users, user);
    if(!u){
        tUsuario nu; memset(&nu,0,sizeof(nu));
        snprintf(nu.username, sizeof(nu.username), "%s", user);
        if(!listaU_push_front(&g_users, &nu)){ puts("Sin memoria."); ui_press_enter(); return; }
        u = listaU_find_username(g_users, user);
        puts("Usuario creado.");
    }else{
        puts("Usuario encontrado.");
    }
    g_user = u;
    puts("");
    usuario_print(g_user);
    ui_press_enter();
}


static void do_buscar_bst(void){
    char artista[50], titulo[50];
    ui_flush_input();
    printf("Artista: "); if(scanf(" %49[^\n]", artista)!=1) return;
    ui_flush_input();
    printf("Titulo:  "); if(scanf(" %49[^\n]", titulo)!=1) return;
    tCancion* c = bst_find(g_idx, artista, titulo);
    if(!c) puts("No encontrada.");
    else   cancion_print(c);
}

static void do_buscar_trie(void){
    char pref[50];
    ui_flush_input();
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
        ui_header("Playlist");
        puts("1) Agregar por prefijo (Trie)");
        puts("2) Quitar por id");
        puts("3) Mostrar playlist");
        puts("4) Quitar por prefijo (Trie)");
        puts("0) Volver");
        ui_flush_input();
        printf("> "); if(scanf("%d",&op)!=1) return;

        if(op==1){
            ui_header("Playlist > Agregar por prefijo");
            int id = elegir_cancion_por_prefijo();
            if(id >= 0){
                tCancion* c = lista_find_by_id(g_biblio, id);
                if(c && lista_push_front(&g_playlist, *c)){
                    tAccion a = { .action=1, .song_id=id, .aux=0 };
                    pila_push(&g_undo, a);
                    puts("Agregada desde Trie.");
                }else puts("No se pudo agregar.");
            }
            ui_press_enter();
        }else if(op==2){
            ui_header("Playlist > Quitar por id");
            ui_flush_input();
            int id; printf("id: "); if(scanf("%d",&id)==1){
                if(lista_remove_by_id(&g_playlist, id)){
                    tAccion a = { .action=2, .song_id=id, .aux=0 };
                    pila_push(&g_undo, a);
                    puts("Quitada.");
                }else puts("No estaba en playlist.");
            }
            ui_press_enter();
        }else if(op==3){
            ui_header("Playlist > Mostrar");
            mostrar_lista(g_playlist, "Playlist actual");
            ui_press_enter();
        }else if(op==4){
            ui_header("Playlist > Quitar por prefijo");
            int id = elegir_cancion_por_prefijo_en_playlist();
            if(id >= 0){
                if(lista_remove_by_id(&g_playlist, id)){
                    tAccion a = { .action=2, .song_id=id, .aux=0 };
                    pila_push(&g_undo, a);
                    puts("Quitada (Trie).");
                }else puts("Esa canción no está en tu playlist.");
            }
            ui_press_enter();
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
        ui_flush_input();
        printf("> "); if(scanf("%d",&op)!=1) return;
        if(op==1){
            ui_flush_input();
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
            printf("> Reproduciendo: "); cancion_print(c);
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
    ui_flush_input();
    int id,k; printf("Desde id: "); if(scanf("%d",&id)!=1) return;
    ui_flush_input();
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
        op = tui_menu_select();
        ui_clear();
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
