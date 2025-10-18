#include "library.h"
#include "player.h"

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

#define SONGS_DIR "canciones"
#define METADATA_FILE "data/metadata.txt"

#define COLOR_RESET "\033[0m"
#define COLOR_PRIMARY "\033[38;5;45m"
#define COLOR_SECONDARY "\033[38;5;252m"
#define COLOR_ACCENT "\033[38;5;219m"
#define COLOR_SUCCESS "\033[38;5;120m"
#define COLOR_ERROR "\033[38;5;203m"
#define COLOR_WARNING "\033[38;5;221m"
#define STYLE_BOLD "\033[1m"
#define STYLE_DIM "\033[2m"

typedef struct {
    char text[256];
    const char *color;
} StatusMessage;

static void enable_ansi_colors(void);
static void clear_screen(void);
static void print_header(const SongLibrary *library, size_t last_played);
static void render_menu(const SongLibrary *library, size_t last_played, const StatusMessage *status);
static void update_status(StatusMessage *status, const char *color, const char *fmt, ...);
static int read_choice(void);
static size_t request_index(const SongLibrary *library);
static void wait_for_enter(void);
static void show_song_list_ui(const SongLibrary *library, size_t highlight_index);
static bool show_recommendations_ui(const SongLibrary *library, size_t current_index);

static void enable_ansi_colors(void) {
#ifdef _WIN32
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE) {
        return;
    }
    DWORD mode = 0;
    if (!GetConsoleMode(handle, &mode)) {
        return;
    }
    if ((mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) == 0) {
        SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
}

static void clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

static void update_status(StatusMessage *status, const char *color, const char *fmt, ...) {
    if (!status || !fmt) {
        return;
    }
    status->color = color ? color : COLOR_SECONDARY;
    va_list args;
    va_start(args, fmt);
    vsnprintf(status->text, sizeof(status->text), fmt, args);
    va_end(args);
}

static void print_header(const SongLibrary *library, size_t last_played) {
    printf(COLOR_ACCENT STYLE_BOLD "╔════════════════════════════════════════════════════════╗\n");
    printf("║%s%-54s%s║\n", COLOR_PRIMARY STYLE_BOLD, "  Reproductor MP3 — siempre visible", COLOR_ACCENT STYLE_BOLD);
    printf("╚════════════════════════════════════════════════════════╝%s\n", COLOR_RESET);

    size_t total = library ? library->count : 0;
    printf("%sCanciones disponibles:%s %zu\n", COLOR_SECONDARY, COLOR_RESET, total);
    if (library && last_played < library->count) {
        const Song *song = &library->songs[last_played];
        printf("%sÚltima reproducción:%s %s — %s\n", COLOR_SECONDARY, COLOR_RESET, song->title, song->artist);
    } else {
        printf("%sÚltima reproducción:%s %s\n", COLOR_SECONDARY, COLOR_RESET, "Todavía no reproduciste canciones en esta sesión.");
    }
    if (library) {
        printf("%sCarpeta monitoreada:%s %s\n", COLOR_SECONDARY, COLOR_RESET, library->songs_dir);
    }
    printf("\n");
}

static void render_menu(const SongLibrary *library, size_t last_played, const StatusMessage *status) {
    clear_screen();
    print_header(library, last_played);
    if (status && status->text[0] != '\0') {
        const char *color = status->color ? status->color : COLOR_SECONDARY;
        printf("%s%s%s\n\n", color, status->text, COLOR_RESET);
    }
    printf("%s%sMenú principal%s\n", COLOR_PRIMARY, STYLE_BOLD, COLOR_RESET);
    printf("%s[1]%s Explorar biblioteca\n", COLOR_ACCENT, COLOR_RESET);
    printf("%s[2]%s Reproducir canción\n", COLOR_ACCENT, COLOR_RESET);
    printf("%s[3]%s Ver recomendaciones\n", COLOR_ACCENT, COLOR_RESET);
    printf("%s[4]%s Reescanear carpeta\n", COLOR_ACCENT, COLOR_RESET);
    printf("%s[5]%s Salir\n\n", COLOR_ACCENT, COLOR_RESET);
    printf("%s%sElegí una opción y presioná Enter.%s\n", COLOR_SECONDARY, STYLE_DIM, COLOR_RESET);
    printf("%s> %s", COLOR_ACCENT, COLOR_RESET);
    fflush(stdout);
}

static int read_choice(void) {
    char buffer[32];
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return -1;
    }
    char *endptr = NULL;
    long value = strtol(buffer, &endptr, 10);
    if (endptr == buffer) {
        return 0;
    }
    return (int)value;
}

static size_t request_index(const SongLibrary *library) {
    if (!library) {
        return (size_t)-1;
    }
    char buffer[32];
    printf("%sSeleccioná el número de canción:%s ", COLOR_PRIMARY, COLOR_RESET);
    fflush(stdout);
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return (size_t)-1;
    }
    long idx = strtol(buffer, NULL, 10);
    if (idx <= 0 || (size_t)idx > library->count) {
        printf("%sSelección inválida.%s\n", COLOR_ERROR, COLOR_RESET);
        return (size_t)-1;
    }
    return (size_t)(idx - 1);
}

static void wait_for_enter(void) {
    printf("%s%sPresioná Enter para volver al menú...%s\n", COLOR_SECONDARY, STYLE_DIM, COLOR_RESET);
    fflush(stdout);
    char buffer[8];
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        clearerr(stdin);
    }
}

static void show_song_list_ui(const SongLibrary *library, size_t highlight_index) {
    if (!library) {
        return;
    }
    clear_screen();
    print_header(library, highlight_index);
    if (library->count == 0) {
        printf("%sNo hay canciones para mostrar.%s\n\n", COLOR_WARNING, COLOR_RESET);
        wait_for_enter();
        return;
    }
    printf("%s%sExplorá tu biblioteca:%s\n\n", COLOR_PRIMARY, STYLE_BOLD, COLOR_RESET);
    library_print(library, highlight_index);
    wait_for_enter();
}

static bool show_recommendations_ui(const SongLibrary *library, size_t current_index) {
    if (!library) {
        return false;
    }
    clear_screen();
    print_header(library, current_index);
    if (library->count == 0) {
        printf("%sPrimero agregá canciones a la carpeta monitoreada.%s\n\n", COLOR_WARNING, COLOR_RESET);
        wait_for_enter();
        return false;
    }
    if (current_index >= library->count) {
        printf("%sReproducí una canción para obtener recomendaciones personalizadas.%s\n\n", COLOR_WARNING, COLOR_RESET);
        wait_for_enter();
        return false;
    }

    size_t indices[10];
    size_t found = library_recommend_by_artist(library, current_index, indices, 10);
    if (found == 0) {
        printf("%sNo hay otras canciones del artista %s.%s\n\n", COLOR_WARNING, library->songs[current_index].artist, COLOR_RESET);
        wait_for_enter();
        return false;
    }

    const Song *current_song = &library->songs[current_index];
    printf("%s%sTe recomendamos también:%s %s — %s\n\n", COLOR_PRIMARY, STYLE_BOLD, COLOR_RESET, current_song->title, current_song->artist);
    for (size_t i = 0; i < found; ++i) {
        size_t idx = indices[i];
        printf("%s[%zu]%s %s — %s\n", COLOR_ACCENT, idx + 1, COLOR_RESET, library->songs[idx].title, library->songs[idx].artist);
    }
    printf("\n");
    wait_for_enter();
    return true;
}

int main(void) {
    enable_ansi_colors();

    SongLibrary library;
    AudioPlayer player;
    StatusMessage status = {{0}, COLOR_SECONDARY};

    if (player_init(&player) != 0) {
        fprintf(stderr, "%sNo se pudo inicializar el subsistema de audio.%s\n", COLOR_ERROR, COLOR_RESET);
        return EXIT_FAILURE;
    }

    if (library_init(&library, SONGS_DIR, METADATA_FILE) != 0) {
        fprintf(stderr, "%sNo se pudo inicializar la biblioteca de canciones.%s\n", COLOR_ERROR, COLOR_RESET);
        player_shutdown(&player);
        return EXIT_FAILURE;
    }

    update_status(&status, COLOR_SUCCESS, "Listo para reproducir. Hay %zu canciones disponibles.", library.count);

    bool running = true;
    size_t last_played = (size_t)-1;

    while (running) {
        render_menu(&library, last_played, &status);
        int choice = read_choice();
        if (choice == -1) {
            running = false;
            break;
        }
        switch (choice) {
            case 1:
                if (library.count == 0) {
                    update_status(&status, COLOR_WARNING, "No hay canciones en %s. Agregá archivos MP3.", library.songs_dir);
                } else {
                    show_song_list_ui(&library, last_played);
                    update_status(&status, COLOR_SUCCESS, "Biblioteca mostrada (%zu canciones).", library.count);
                }
                break;
            case 2:
                if (library.count == 0) {
                    update_status(&status, COLOR_WARNING, "No hay canciones en %s. Agregá archivos MP3.", library.songs_dir);
                    break;
                }
                clear_screen();
                print_header(&library, last_played);
                printf("%sElegí una canción para reproducir.%s\n\n", COLOR_SECONDARY, COLOR_RESET);
                library_print(&library, last_played);
                {
                    size_t index = request_index(&library);
                    if (index == (size_t)-1) {
                        update_status(&status, COLOR_WARNING, "No se seleccionó una canción válida.");
                        wait_for_enter();
                        break;
                    }
                    printf("\n");
                    int play_result = player_play_song(&player, &library.songs[index], library.songs_dir);
                    if (play_result == 0) {
                        last_played = index;
                        update_status(&status, COLOR_SUCCESS, "Terminó %s — %s.", library.songs[index].title, library.songs[index].artist);
                    } else {
                        update_status(&status, COLOR_ERROR, "Hubo un problema al reproducir la canción.");
                    }
                    wait_for_enter();
                }
                break;
            case 3: {
                bool shown = show_recommendations_ui(&library, last_played);
                if (shown) {
                    if (last_played < library.count) {
                        update_status(&status, COLOR_SUCCESS, "Más canciones de %s listas para sonar.", library.songs[last_played].artist);
                    } else {
                        update_status(&status, COLOR_SUCCESS, "Explorá las recomendaciones mostradas.");
                    }
                } else {
                    if (library.count == 0) {
                        update_status(&status, COLOR_WARNING, "Primero agregá canciones a %s.", library.songs_dir);
                    } else if (last_played >= library.count) {
                        update_status(&status, COLOR_WARNING, "Reproducí una canción para recibir recomendaciones.");
                    } else {
                        update_status(&status, COLOR_WARNING, "No hay más canciones del artista %s.", library.songs[last_played].artist);
                    }
                }
                break;
            }
            case 4:
                if (library_scan(&library) == 0) {
                    update_status(&status, COLOR_SUCCESS, "Biblioteca actualizada. Canciones disponibles: %zu.", library.count);
                } else {
                    update_status(&status, COLOR_ERROR, "Ocurrió un error al reescanear la biblioteca.");
                }
                break;
            case 5:
                running = false;
                break;
            default:
                update_status(&status, COLOR_ERROR, "Opción inválida. Intentá nuevamente.");
                break;
        }
    }

    clear_screen();
    printf("%s%sGracias por usar el reproductor MP3.%s\n", COLOR_PRIMARY, STYLE_BOLD, COLOR_RESET);

    library_free(&library);
    player_shutdown(&player);

    return EXIT_SUCCESS;
}
