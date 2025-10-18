#include "library.h"
#include "player.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SONGS_DIR "canciones"
#define METADATA_FILE "data/metadata.txt"

static void print_menu(void) {
    printf("==============================\n");
    printf(" Reproductor de MP3 minimalista\n");
    printf("==============================\n");
    printf("1. Listar canciones\n");
    printf("2. Reproducir una canción\n");
    printf("3. Recomendar canciones del mismo artista\n");
    printf("4. Reescanear carpeta de canciones\n");
    printf("5. Salir\n");
    printf("> ");
    fflush(stdout);
}

static int read_choice(void) {
    char buffer[32];
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return -1;
    }
    return (int)strtol(buffer, NULL, 10);
}

static size_t request_index(const SongLibrary *library) {
    char buffer[32];
    printf("Ingrese el número de canción: ");
    fflush(stdout);
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return (size_t)-1;
    }
    long idx = strtol(buffer, NULL, 10);
    if (idx <= 0 || (size_t)idx > library->count) {
        printf("Selección inválida.\n");
        return (size_t)-1;
    }
    return (size_t)(idx - 1);
}

static void show_recommendations(const SongLibrary *library, size_t current_index) {
    if (library->count == 0) {
        printf("No hay canciones cargadas.\n");
        return;
    }
    if (current_index >= library->count) {
        printf("Todavía no se reprodujo ninguna canción en esta sesión.\n");
        return;
    }

    size_t indices[10];
    size_t found = library_recommend_by_artist(library, current_index, indices, 10);
    if (found == 0) {
        printf("No hay otras canciones del artista %s.\n", library->songs[current_index].artist);
        return;
    }

    printf("Recomendaciones para %s — %s:\n", library->songs[current_index].title, library->songs[current_index].artist);
    for (size_t i = 0; i < found; ++i) {
        size_t idx = indices[i];
        printf(" - [%zu] %s\n", idx + 1, library->songs[idx].title);
    }
}

int main(void) {
    SongLibrary library;
    AudioPlayer player;

    if (player_init(&player) != 0) {
        fprintf(stderr, "No se pudo inicializar el subsistema de audio.\n");
        return EXIT_FAILURE;
    }

    if (library_init(&library, SONGS_DIR, METADATA_FILE) != 0) {
        fprintf(stderr, "No se pudo inicializar la biblioteca de canciones.\n");
        player_shutdown(&player);
        return EXIT_FAILURE;
    }

    bool running = true;
    size_t last_played = (size_t)-1;

    while (running) {
        print_menu();
        int choice = read_choice();
        switch (choice) {
            case 1:
                library_print(&library);
                break;
            case 2:
                if (library.count == 0) {
                    printf("No hay canciones en %s. Agregá archivos MP3 y reescaneá.\n", SONGS_DIR);
                    break;
                }
                library_print(&library);
                {
                    size_t index = request_index(&library);
                    if (index == (size_t)-1) {
                        break;
                    }
                    if (player_play_song(&player, &library.songs[index], library.songs_dir) == 0) {
                        last_played = index;
                    }
                }
                break;
            case 3:
                show_recommendations(&library, last_played);
                break;
            case 4:
                if (library_scan(&library) == 0) {
                    printf("Biblioteca actualizada. Canciones disponibles: %zu\n", library.count);
                } else {
                    printf("Ocurrió un error al reescanear la biblioteca.\n");
                }
                break;
            case 5:
                running = false;
                break;
            default:
                printf("Opción inválida.\n");
                break;
        }
    }

    library_free(&library);
    player_shutdown(&player);

    printf("Hasta luego.\n");
    return EXIT_SUCCESS;
}
