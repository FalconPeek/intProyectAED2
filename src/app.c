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
#include "persist_userstats_txt.h"
#include "usuarios_lista.h"
#include "usuario.h"
#include "cancion.h"
/* ===== TUI ANSI sin dependencias ===== */
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static void ui_trim_newline(char* s){
    if(!s) return;
    size_t len = strlen(s);
    if(len && s[len-1] == '\n') s[len-1] = '\0';
}

static int ui_read_line(char* buffer, size_t len){
    if(!fgets(buffer, len, stdin)) return 0;
    ui_trim_newline(buffer);
    return 1;
}

static int ui_read_string_prompt(const char* prompt, char* buffer, size_t len){
    if(prompt) printf("%s", prompt);
    if(!ui_read_line(buffer, len)) return 0;
    return buffer[0] != '\0';
}

static int ui_read_int_prompt(const char* prompt, int* out){
    char line[64];
    if(prompt) printf("%s", prompt);
    if(!ui_read_line(line, sizeof(line))) return 0;
    char* end = NULL;
    long val = strtol(line, &end, 10);
    if(end == line || (end && *end != '\0')) return 0;
    *out = (int)val;
    return 1;
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
#define PATH_BIBLIO        "data/biblioteca.txt"
#define PATH_USUARIOS      "data/usuarios.txt"
#define PATH_PLAYLIST_DIR  "data/playlist"
#define PATH_PLAYLIST_FILE "data/playlist/miplaylist.txt"
#define PATH_USERSTATS_DIR "data/userstats"

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
static tUserStats g_user_stats;
static int g_userstats_loaded = 0;

static int g_dirty_biblio = 0;
static int g_dirty_users = 0;
static int g_dirty_playlist = 0;
static int g_dirty_userstats = 0;

static int ensure_directory(const char* path){
    struct stat st;
    if(stat(path, &st) == 0){
        return S_ISDIR(st.st_mode);
    }
    if(errno == ENOENT){
        return mkdir(path, 0755) == 0;
    }
    return 0;
}

static void ensure_data_directories(void){
    if(!ensure_directory(PATH_PLAYLIST_DIR)){
        fprintf(stderr, "WARN: No se pudo asegurar directorio %s\n", PATH_PLAYLIST_DIR);
    }
    if(!ensure_directory(PATH_USERSTATS_DIR)){
        fprintf(stderr, "WARN: No se pudo asegurar directorio %s\n", PATH_USERSTATS_DIR);
    }
}

static void userstats_path(const char* username, char* out, size_t len){
    snprintf(out, len, "%s/%s.txt", PATH_USERSTATS_DIR, username);
}

static void reset_biblio(void){
    lista_free(&g_biblio);
    bst_free(&g_idx);
    trie_free(&g_trie);
    g_biblio = NULL;
    g_idx = NULL;
    g_trie = NULL;
    bst_init(&g_idx);
    trie_init(&g_trie);
}

static int guardar_userstats_actual(void){
    if(!g_user || !g_userstats_loaded) return 1;
    ensure_directory(PATH_USERSTATS_DIR);
    char path[PATH_MAX];
    userstats_path(g_user->username, path, sizeof(path));
    if(!ustats_save_txt(path, &g_user_stats)){
        fprintf(stderr, "ERROR: No se pudo guardar stats de %s\n", g_user->username);
        return 0;
    }
    g_dirty_userstats = 0;
    return 1;
}

static int cargar_userstats_para(tUsuario* u){
    if(!u) return 0;
    if(g_userstats_loaded && g_dirty_userstats){
        if(!guardar_userstats_actual()) return 0;
    }
    ustats_free(&g_user_stats);
    ustats_init(&g_user_stats);
    ensure_directory(PATH_USERSTATS_DIR);
    char path[PATH_MAX];
    userstats_path(u->username, path, sizeof(path));
    if(!ustats_load_txt(path, &g_user_stats)){
        fprintf(stderr, "ERROR: No se pudo cargar stats de %s\n", u->username);
        return 0;
    }
    g_userstats_loaded = 1;
    g_dirty_userstats = 0;
    return 1;
}

static void cargar_playlist_desde_archivo(void){
    lista_free(&g_playlist);
    g_playlist = NULL;
    if(!persist_playlist_load_txt(PATH_PLAYLIST_FILE, &g_playlist)){
        fprintf(stderr, "WARN: No se pudo cargar playlist desde %s\n", PATH_PLAYLIST_FILE);
    }
    g_dirty_playlist = 0;
}

static void guardar_todo(void){
    ensure_data_directories();

    if(g_dirty_biblio){
        if(persist_biblio_save_txt(PATH_BIBLIO, g_biblio)){
            puts("OK: Biblioteca guardada.");
            g_dirty_biblio = 0;
        }else{
            puts("ERROR: al guardar biblioteca.");
        }
    }else{
        puts("Biblioteca sin cambios.");
    }

    if(g_dirty_playlist){
        int n = lista_len(g_playlist);
        if(n > 0){
            tCancion* v = (tCancion*)malloc(sizeof(tCancion)*n);
            if(v){
                lista_to_array(g_playlist, v, n);
                if(persist_playlist_save_txt(PATH_PLAYLIST_FILE, v, n)){
                    puts("OK: Playlist guardada.");
                    g_dirty_playlist = 0;
                }else{
                    puts("ERROR: al guardar playlist.");
                }
                free(v);
            }else{
                puts("Sin memoria para guardar playlist.");
            }
        }else{
            /* playlist vacía -> guardar archivo vacío */
            if(persist_playlist_save_txt(PATH_PLAYLIST_FILE, NULL, 0)){
                puts("Playlist vacía guardada.");
                g_dirty_playlist = 0;
            }
        }
    }else{
        puts("Playlist sin cambios.");
    }

    if(g_dirty_users){
        if(persist_usuarios_save_txt(PATH_USUARIOS, g_users)){
            puts("OK: Usuarios guardados.");
            g_dirty_users = 0;
        }else{
            puts("ERROR: al guardar usuarios.");
        }
    }else{
        puts("Usuarios sin cambios.");
    }

    if(g_userstats_loaded){
        if(g_dirty_userstats){
            if(guardar_userstats_actual()) puts("OK: Stats de usuario guardadas.");
            else puts("ERROR: al guardar stats del usuario.");
        }else{
            puts("Stats de usuario sin cambios.");
        }
    }
}

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
    puts("9) Estadisticas y leaderboards");
    puts("10) Top-N mas reproducidas (global)");
    puts("11) Guardar biblioteca/playlist/usuarios");
    puts("0) Salir");
}

/* Construye grafo: conecta canciones con mismo artista */
static int cmp_cancion_artista(const void* a, const void* b){
    const tCancion* ca = (const tCancion*)a;
    const tCancion* cb = (const tCancion*)b;
    int cmp = strcmp(ca->artista, cb->artista);
    if(cmp != 0) return cmp;
    return (ca->id - cb->id);
}

static void construir_grafo_por_artista(void){
    int n = lista_len(g_biblio);
    if(n <= 0){
        grafo_init(&g_G, 0);
        puts("Biblioteca vacía: grafo limpio.");
        return;
    }

    tCancion* arr = (tCancion*)malloc(sizeof(tCancion) * n);
    if(!arr){
        puts("Sin memoria para grafo.");
        return;
    }
    lista_to_array(g_biblio, arr, n);

    int maxId = -1;
    for(int i=0;i<n;i++) if(arr[i].id > maxId) maxId = arr[i].id;
    grafo_init(&g_G, maxId + 1);

    qsort(arr, n, sizeof(tCancion), cmp_cancion_artista);
    int edges = 0;
    for(int i=1;i<n;i++){
        if(strcmp(arr[i].artista, arr[i-1].artista) == 0){
            grafo_add_edge(&g_G, arr[i].id, arr[i-1].id);
            edges++;
        }
    }
    free(arr);
    printf("OK: Grafo construido (artistas conectados con %d aristas).\n", edges);
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
    if(!ui_read_string_prompt("Prefijo de titulo: ", pref, sizeof(pref))) return -1;

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

    char prompt[64];
    snprintf(prompt, sizeof(prompt), "Elegí una opción (1-%d, 0 para cancelar): ", printed);
    int sel = 0;
    if(!ui_read_int_prompt(prompt, &sel) || sel < 1 || sel > printed){
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
    if(!ui_read_string_prompt("Prefijo de titulo (en playlist): ", pref, sizeof(pref))) return -1;

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

    char prompt[64];
    snprintf(prompt, sizeof(prompt), "Elegí una opción (1-%d, 0 para cancelar): ", printed);
    int sel = 0;
    if(!ui_read_int_prompt(prompt, &sel) || sel<1 || sel>printed){
        puts("Cancelado o selección inválida.");
        return -1;
    }
    return ids[sel-1];
}

static void playlist_buscar(void){
    int n = lista_len(g_playlist);
    if(n <= 0){
        puts("Playlist vacia.");
        return;
    }
    tCancion* arr = (tCancion*)malloc(sizeof(tCancion) * n);
    if(!arr){ puts("Sin memoria."); return; }
    lista_to_array(g_playlist, arr, n);

    int id = 0;
    if(ui_read_int_prompt("ID a buscar (lineal): ", &id)){
        long comps = 0;
        int idx = search_lineal_por_id(arr, n, id, &comps);
        if(idx >= 0){
            printf("Encontrada en posición %d (lineal). Comparaciones: %ld\n", idx, comps);
            cancion_print(&arr[idx]);
        }else{
            printf("ID %d no está en la playlist. Comparaciones: %ld\n", id, comps);
        }
    }else{
        puts("Entrada inválida para ID.");
    }

    tCancion* sorted = (tCancion*)malloc(sizeof(tCancion) * n);
    if(!sorted){ free(arr); puts("Sin memoria."); return; }
    memcpy(sorted, arr, sizeof(tCancion)*n);
    long compsSort = 0, swapsSort = 0;
    sort_por_titulo_burbuja(sorted, n, &compsSort, &swapsSort);

    char titulo[50];
    if(ui_read_string_prompt("Titulo exacto para buscar (binaria): ", titulo, sizeof(titulo))){
        long compsBin = 0;
        int idx = search_binaria_por_titulo(sorted, n, titulo, &compsBin);
        if(idx >= 0){
            printf("Encontrada por binaria en posición %d del vector ordenado.\n", idx);
            cancion_print(&sorted[idx]);
        }else{
            puts("El título no está en la playlist.");
        }
        printf("Métricas -> Ordenamiento burbuja: comps=%ld swaps=%ld | Binaria comps=%ld\n",
               compsSort, swapsSort, compsBin);
    }

    free(sorted);
    free(arr);
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
    "Estadisticas y leaderboards",
    "Top-N mas reproducidas (global)",
    "Guardar biblioteca/playlist/usuarios",
    "Salir"
};
static const int g_menu_count = 12;

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
    reset_biblio();

    if(persist_biblio_load_txt(PATH_BIBLIO, &g_biblio, &g_idx, &g_trie)){
        puts("OK: Biblioteca cargada.");
        g_dirty_biblio = 0;
    }else{
        puts("ERROR: No se pudo cargar biblioteca. Verifique data/biblioteca.txt");
    }

    ui_press_enter();
}


static void do_login(void){
    ui_header("Login de usuario");
    char user[50];
    if(!ui_read_string_prompt("Usuario: ", user, sizeof(user))){
        puts("Entrada invalida.");
        ui_press_enter();
        return;
    }

    if(!g_users) listaU_init(&g_users);

    tUsuario* u = listaU_find_username(g_users, user);
    if(!u){
        tUsuario nu; memset(&nu,0,sizeof(nu));
        snprintf(nu.username, sizeof(nu.username), "%s", user);
        if(!listaU_push_front(&g_users, &nu)){
            puts("Sin memoria.");
            ui_press_enter();
            return;
        }
        u = listaU_find_username(g_users, user);
        puts("Usuario creado.");
        g_dirty_users = 1;
    }else{
        puts("Usuario encontrado.");
    }
    g_user = u;
    if(!cargar_userstats_para(g_user)){
        puts("Advertencia: no se pudieron cargar estadísticas del usuario.");
        ustats_init(&g_user_stats);
        g_userstats_loaded = 1;
    }

    puts("");
    usuario_print(g_user);
    if(g_user){
        double horas = g_user->segundos_escucha_total / 3600.0;
        printf("Tiempo escuchado: %.2f horas (%ld seg)\n", horas, g_user->segundos_escucha_total);
    }
    ui_press_enter();
}


static void do_buscar_bst(void){
    char artista[50], titulo[50];
    if(!ui_read_string_prompt("Artista: ", artista, sizeof(artista))) return;
    if(!ui_read_string_prompt("Titulo:  ", titulo, sizeof(titulo))) return;
    tCancion* c = bst_find(g_idx, artista, titulo);
    if(!c) puts("No encontrada.");
    else   cancion_print(c);
}

static void do_buscar_trie(void){
    char pref[50];
    if(!ui_read_string_prompt("Prefijo de titulo: ", pref, sizeof(pref))) return;
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
        puts("5) Buscar (lineal/binaria)");
        puts("0) Volver");
        if(!ui_read_int_prompt("> ", &op)) return;

        if(op==1){
            ui_header("Playlist > Agregar por prefijo");
            int id = elegir_cancion_por_prefijo();
            if(id >= 0){
                tCancion* c = lista_find_by_id(g_biblio, id);
                if(c && lista_push_front(&g_playlist, *c)){
                    tAccion a = { .action=1, .song_id=id, .aux=0 };
                    pila_push(&g_undo, a);
                    g_dirty_playlist = 1;
                    puts("Agregada desde Trie.");
                }else puts("No se pudo agregar.");
            }
            ui_press_enter();
        }else if(op==2){
            ui_header("Playlist > Quitar por id");
            int id = 0;
            if(ui_read_int_prompt("id: ", &id)){
                if(lista_remove_by_id(&g_playlist, id)){
                    tAccion a = { .action=2, .song_id=id, .aux=0 };
                    pila_push(&g_undo, a);
                    g_dirty_playlist = 1;
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
                    g_dirty_playlist = 1;
                    puts("Quitada (Trie).");
                }else puts("Esa canción no está en tu playlist.");
            }
            ui_press_enter();
        }else if(op==5){
            ui_header("Playlist > Buscar");
            playlist_buscar();
            ui_press_enter();
        }
    }while(op!=0);
}



static void do_undo(void){
    tAccion a;
    if(!pila_pop(&g_undo, &a)){ puts("Nada para deshacer."); return; }
    if(a.action==1){ /* se había agregado -> ahora quitar */
        if(lista_remove_by_id(&g_playlist, a.song_id)){ puts("Undo: se quitó."); g_dirty_playlist = 1; }
        else puts("Undo: no estaba.");
    }else if(a.action==2){ /* se había quitado -> reinsertar */
        tCancion* c = lista_find_by_id(g_biblio, a.song_id);
        if(c && lista_push_front(&g_playlist, *c)){ puts("Undo: se reinsertó."); g_dirty_playlist = 1; }
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
        if(!ui_read_int_prompt("> ", &op)) return;
        if(op==1){
            int id = 0;
            if(!ui_read_int_prompt("id: ", &id)) continue;
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
            g_dirty_biblio = 1;
            if(g_user){
                g_user->reproducciones_totales++;
                g_user->segundos_escucha_total += c->duracion_seg;
                g_dirty_users = 1;
                if(ustats_add_play(&g_user_stats, c->id, c->duracion_seg)){
                    g_dirty_userstats = 1;
                }else{
                    puts("Advertencia: no se pudieron actualizar las stats detalladas.");
                }
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
    int id = 0, k = 0;
    if(!ui_read_int_prompt("Desde id: ", &id)) return;
    if(!ui_read_int_prompt("Cuantos? ", &k) || k <= 0){ puts("Cantidad inválida."); return; }
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

static int cmp_stats_desc(const void* a, const void* b){
    const tUserSongStat* sa = (const tUserSongStat*)a;
    const tUserSongStat* sb = (const tUserSongStat*)b;
    if(sa->plays != sb->plays) return (sb->plays - sa->plays);
    if(sa->segs < sb->segs) return 1;
    if(sa->segs > sb->segs) return -1;
    return sa->song_id - sb->song_id;
}

static int cmp_usuario_por_plays_desc(const void* a, const void* b){
    const tUsuario* ua = (const tUsuario*)a;
    const tUsuario* ub = (const tUsuario*)b;
    if(ua->reproducciones_totales != ub->reproducciones_totales)
        return (ub->reproducciones_totales - ua->reproducciones_totales);
    if(ua->segundos_escucha_total < ub->segundos_escucha_total) return 1;
    if(ua->segundos_escucha_total > ub->segundos_escucha_total) return -1;
    return strcmp(ua->username, ub->username);
}

static void do_estadisticas(void){
    ui_header("Estadisticas & Leaderboards");

    if(!g_users){
        puts("No hay usuarios cargados.");
        ui_press_enter();
        return;
    }

    if(g_user){
        printf("Usuario activo: %s\n", g_user->username);
        printf("Total reproducciones: %d\n", g_user->reproducciones_totales);
        double horas = g_user->segundos_escucha_total / 3600.0;
        printf("Tiempo escuchado: %.2f horas (%ld seg)\n", horas, g_user->segundos_escucha_total);
        if(g_userstats_loaded && g_user_stats.n > 0){
            tUserSongStat* arr = (tUserSongStat*)malloc(sizeof(tUserSongStat)*g_user_stats.n);
            if(arr){
                memcpy(arr, g_user_stats.v, sizeof(tUserSongStat)*g_user_stats.n);
                qsort(arr, g_user_stats.n, sizeof(tUserSongStat), cmp_stats_desc);
                int limit = g_user_stats.n < 5 ? g_user_stats.n : 5;
                puts("Top canciones del usuario:");
                for(int i=0;i<limit;i++){
                    tCancion* c = lista_find_by_id(g_biblio, arr[i].song_id);
                    if(c){
                        printf("%d) ", i+1);
                        cancion_print(c);
                    }else{
                        printf("%d) Cancion id=%d (plays=%d, seg=%ld)\n", i+1, arr[i].song_id, arr[i].plays, arr[i].segs);
                    }
                }
                free(arr);
            }
            printf("Canciones únicas escuchadas: %d\n", g_user_stats.n);
        }else{
            puts("Aún no hay estadísticas detalladas para este usuario.");
        }
    }else{
        puts("No hay usuario activo.");
    }

    int total = listaU_len(g_users);
    if(total > 0){
        tUsuario* arrU = (tUsuario*)malloc(sizeof(tUsuario)*total);
        if(arrU){
            int idx = 0;
            for(tListaU* it = g_users; it; it = it->sig) arrU[idx++] = it->info;
            qsort(arrU, total, sizeof(tUsuario), cmp_usuario_por_plays_desc);
            int limit = total < 5 ? total : 5;
            puts("\nLeaderboard global (reproducciones):");
            for(int i=0;i<limit;i++){
                double horas = arrU[i].segundos_escucha_total / 3600.0;
                printf("%d) %-12s plays=%d horas=%.2f\n", i+1, arrU[i].username,
                       arrU[i].reproducciones_totales, horas);
            }
            free(arrU);
        }
    }

    ui_press_enter();
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
    ui_header("Guardar estado");
    guardar_todo();
    ui_press_enter();
}

/* --- Loop principal --- */
void app_run(void){
    banner();
    ensure_data_directories();

    cola_init(&g_queue);
    pila_init(&g_undo);
    bst_init(&g_idx);
    trie_init(&g_trie);
    ustats_init(&g_user_stats);

    if(!g_users) listaU_init(&g_users);
    if(!persist_usuarios_load_txt(PATH_USUARIOS, &g_users)){
        puts("WARN: No se pudieron cargar usuarios existentes.");
    }
    g_dirty_users = 0;
    g_dirty_biblio = 0;
    g_dirty_playlist = 0;
    g_dirty_userstats = 0;
    g_userstats_loaded = 0;
    g_user = NULL;

    cargar_playlist_desde_archivo();

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
            case 9: do_estadisticas(); break;
            case 10: do_top(); break;
            case 11: do_guardar(); break;
            case 0:  break;
            default: puts("Opcion invalida.");
        }
    }while(op!=0);

    if(g_dirty_biblio || g_dirty_playlist || g_dirty_users || (g_userstats_loaded && g_dirty_userstats)){
        puts("Guardando cambios pendientes antes de salir...");
        guardar_todo();
    }

    /* liberar memoria */
    lista_free(&g_playlist);
    lista_free(&g_biblio);
    bst_free(&g_idx);
    trie_free(&g_trie);
    cola_free(&g_queue);
    pila_free(&g_undo);
    listaU_free(&g_users);
    ustats_free(&g_user_stats);
}
