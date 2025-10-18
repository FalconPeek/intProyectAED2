#include "library.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct {
    char filename[512];
    char title[LIBRARY_MAX_FIELD];
    char artist[LIBRARY_MAX_FIELD];
} MetadataEntry;

static int ensure_directory(const char *path) {
#ifdef _WIN32
    if (_mkdir(path) == 0) {
        return 0;
    }
    if (errno == EEXIST) {
        return 0;
    }
    return errno == EEXIST ? 0 : -1;
#else
    if (mkdir(path, 0755) == 0) {
        return 0;
    }
    if (errno == EEXIST) {
        return 0;
    }
    return errno == EEXIST ? 0 : -1;
#endif
}

static void trim_newline(char *str) {
    if (!str) {
        return;
    }
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

static int ends_with_mp3(const char *name) {
    size_t len = strlen(name);
    if (len < 4) {
        return 0;
    }
    const char *suffix = name + len - 4;
    return tolower((unsigned char)suffix[0]) == '.' &&
           tolower((unsigned char)suffix[1]) == 'm' &&
           tolower((unsigned char)suffix[2]) == 'p' &&
           tolower((unsigned char)suffix[3]) == '3';
}

static MetadataEntry *load_metadata(const char *metadata_path, size_t *out_count) {
    FILE *fp = fopen(metadata_path, "r");
    if (!fp) {
        *out_count = 0;
        return NULL;
    }

    size_t capacity = 8;
    size_t count = 0;
    MetadataEntry *entries = malloc(capacity * sizeof(MetadataEntry));
    if (!entries) {
        fclose(fp);
        *out_count = 0;
        return NULL;
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        trim_newline(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        char *first = strchr(line, '|');
        char *second = first ? strchr(first + 1, '|') : NULL;
        if (!first || !second) {
            continue;
        }
        *first = '\0';
        *second = '\0';
        if (count == capacity) {
            capacity *= 2;
            MetadataEntry *tmp = realloc(entries, capacity * sizeof(MetadataEntry));
            if (!tmp) {
                free(entries);
                fclose(fp);
                *out_count = 0;
                return NULL;
            }
            entries = tmp;
        }
        snprintf(entries[count].filename, sizeof(entries[count].filename), "%s", line);
        snprintf(entries[count].title, sizeof(entries[count].title), "%s", first + 1);
        snprintf(entries[count].artist, sizeof(entries[count].artist), "%s", second + 1);
        count++;
    }

    fclose(fp);
    *out_count = count;
    return entries;
}

static int save_metadata(const char *metadata_path, const MetadataEntry *entries, size_t count) {
    FILE *fp = fopen(metadata_path, "w");
    if (!fp) {
        return -1;
    }
    for (size_t i = 0; i < count; ++i) {
        fprintf(fp, "%s|%s|%s\n", entries[i].filename, entries[i].title, entries[i].artist);
    }
    fclose(fp);
    return 0;
}

static MetadataEntry *find_entry(MetadataEntry *entries, size_t count, const char *filename) {
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(entries[i].filename, filename) == 0) {
            return &entries[i];
        }
    }
    return NULL;
}

static void ensure_capacity(SongLibrary *library) {
    if (library->count < library->capacity) {
        return;
    }
    size_t new_capacity = library->capacity == 0 ? 8 : library->capacity * 2;
    Song *tmp = realloc(library->songs, new_capacity * sizeof(Song));
    if (!tmp) {
        fprintf(stderr, "Error: memoria insuficiente para canciones.\n");
        exit(EXIT_FAILURE);
    }
    library->songs = tmp;
    library->capacity = new_capacity;
}

static void normalize_default(char *dest, size_t dest_size, const char *fallback) {
    if (dest[0] == '\0') {
        snprintf(dest, dest_size, "%s", fallback);
    }
}

int library_init(SongLibrary *library, const char *songs_dir, const char *metadata_path) {
    if (!library || !songs_dir || !metadata_path) {
        return -1;
    }
    memset(library, 0, sizeof(*library));
    snprintf(library->songs_dir, sizeof(library->songs_dir), "%s", songs_dir);
    snprintf(library->metadata_path, sizeof(library->metadata_path), "%s", metadata_path);

    if (ensure_directory(songs_dir) != 0) {
        perror("No se pudo crear el directorio de canciones");
        return -1;
    }

    char metadata_dir[512];
    snprintf(metadata_dir, sizeof(metadata_dir), "%s", metadata_path);
    char *last_sep = strrchr(metadata_dir, '/');
#ifdef _WIN32
    char *last_sep_win = strrchr(metadata_dir, '\\');
    if (!last_sep || (last_sep_win && last_sep_win > last_sep)) {
        last_sep = last_sep_win;
    }
#endif
    if (last_sep) {
        *last_sep = '\0';
        if (metadata_dir[0] != '\0') {
            if (ensure_directory(metadata_dir) != 0) {
                perror("No se pudo crear el directorio de metadatos");
                return -1;
            }
        }
    }

    return library_scan(library);
}

void library_free(SongLibrary *library) {
    if (!library) {
        return;
    }
    free(library->songs);
    library->songs = NULL;
    library->count = 0;
    library->capacity = 0;
}

static void prompt_metadata(const char *filename, MetadataEntry *entry) {
    printf("\nNueva canción detectada: %s\n", filename);
    printf("Título (enter para usar nombre de archivo): ");
    fflush(stdout);
    if (fgets(entry->title, sizeof(entry->title), stdin)) {
        trim_newline(entry->title);
    } else {
        entry->title[0] = '\0';
    }

    printf("Artista (enter para desconocido): ");
    fflush(stdout);
    if (fgets(entry->artist, sizeof(entry->artist), stdin)) {
        trim_newline(entry->artist);
    } else {
        entry->artist[0] = '\0';
    }

    normalize_default(entry->title, sizeof(entry->title), filename);
    normalize_default(entry->artist, sizeof(entry->artist), "Artista desconocido");
}

static int add_song(SongLibrary *library, const char *filename, const MetadataEntry *entry) {
    ensure_capacity(library);
    Song *song = &library->songs[library->count++];
    snprintf(song->filename, sizeof(song->filename), "%s", filename);
    snprintf(song->title, sizeof(song->title), "%s", entry->title);
    snprintf(song->artist, sizeof(song->artist), "%s", entry->artist);
    return 0;
}

int library_scan(SongLibrary *library) {
    if (!library) {
        return -1;
    }

    MetadataEntry *entries = NULL;
    size_t entry_count = 0;
    entries = load_metadata(library->metadata_path, &entry_count);
    int metadata_changed = 0;

    library->count = 0;

#ifdef _WIN32
    char search_path[PATH_MAX];
    snprintf(search_path, sizeof(search_path), "%s\\*.mp3", library->songs_dir);
    WIN32_FIND_DATAA data;
    HANDLE hFind = FindFirstFileA(search_path, &data);
    if (hFind == INVALID_HANDLE_VALUE) {
        free(entries);
        return 0;
    }
    do {
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        const char *filename = data.cFileName;
        if (!ends_with_mp3(filename)) {
            continue;
        }
        MetadataEntry *entry = find_entry(entries, entry_count, filename);
        MetadataEntry temp;
        if (!entry) {
            memset(&temp, 0, sizeof(temp));
            snprintf(temp.filename, sizeof(temp.filename), "%s", filename);
            prompt_metadata(filename, &temp);
            MetadataEntry *tmp_entries = realloc(entries, (entry_count + 1) * sizeof(MetadataEntry));
            if (!tmp_entries) {
                free(entries);
                FindClose(hFind);
                return -1;
            }
            entries = tmp_entries;
            entries[entry_count] = temp;
            entry = &entries[entry_count];
            entry_count++;
            metadata_changed = 1;
        }
        add_song(library, filename, entry);
    } while (FindNextFileA(hFind, &data));
    FindClose(hFind);
#else
    DIR *dir = opendir(library->songs_dir);
    if (!dir) {
        perror("No se pudo abrir el directorio de canciones");
        free(entries);
        return -1;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
#ifdef DT_DIR
        if (entry->d_type == DT_DIR) {
            continue;
        }
#endif
        if (!ends_with_mp3(entry->d_name)) {
            continue;
        }
        MetadataEntry *meta = find_entry(entries, entry_count, entry->d_name);
        MetadataEntry temp;
        if (!meta) {
            memset(&temp, 0, sizeof(temp));
            snprintf(temp.filename, sizeof(temp.filename), "%s", entry->d_name);
            prompt_metadata(entry->d_name, &temp);
            MetadataEntry *tmp_entries = realloc(entries, (entry_count + 1) * sizeof(MetadataEntry));
            if (!tmp_entries) {
                free(entries);
                closedir(dir);
                return -1;
            }
            entries = tmp_entries;
            entries[entry_count] = temp;
            meta = &entries[entry_count];
            entry_count++;
            metadata_changed = 1;
        }
        add_song(library, entry->d_name, meta);
    }
    closedir(dir);
#endif

    if (metadata_changed) {
        if (save_metadata(library->metadata_path, entries, entry_count) != 0) {
            fprintf(stderr, "Advertencia: no se pudo guardar metadata en %s\n", library->metadata_path);
        }
    }

    free(entries);
    return 0;
}

#define COLOR_RESET "\033[0m"
#define COLOR_PRIMARY "\033[38;5;45m"
#define COLOR_SECONDARY "\033[38;5;252m"
#define COLOR_HIGHLIGHT "\033[38;5;213m"
#define COLOR_WARNING "\033[38;5;221m"
#define STYLE_BOLD "\033[1m"
#define STYLE_DIM "\033[2m"

void library_print(const SongLibrary *library, size_t highlight_index) {
    if (!library) {
        return;
    }

    printf("%s%sBiblioteca actual%s\n", COLOR_PRIMARY, STYLE_BOLD, COLOR_RESET);
    printf("%s%sTotal: %zu canciones%s\n", COLOR_SECONDARY, STYLE_DIM, library->count, COLOR_RESET);
    printf("%s%-5s %-48s %-30s%s\n", COLOR_SECONDARY, "#", "Título", "Artista", COLOR_RESET);
    printf("%s%-5s %-48s %-30s%s\n", COLOR_SECONDARY, "────", "────────────────────────────────────────────────", "────────────────────────────", COLOR_RESET);

    int has_highlight = highlight_index < library->count;
    for (size_t i = 0; i < library->count; ++i) {
        int is_highlight = has_highlight && i == highlight_index;
        const char *row_color = is_highlight ? COLOR_HIGHLIGHT : COLOR_SECONDARY;
        const char *row_style = is_highlight ? STYLE_BOLD : "";
        const char *marker = is_highlight ? "▶" : " ";
        printf("%s%s%3zu%s %-48.48s %-30.30s%s\n", row_color, row_style, i + 1, marker, library->songs[i].title, library->songs[i].artist, COLOR_RESET);
    }

    if (library->count == 0) {
        printf("%s%sNo hay canciones en la biblioteca.%s\n", COLOR_WARNING, STYLE_DIM, COLOR_RESET);
    }

    printf("\n");
}

size_t library_recommend_by_artist(const SongLibrary *library, size_t current_index, size_t *output_indices, size_t max_results) {
    if (!library || current_index >= library->count || max_results == 0) {
        return 0;
    }
    const char *artist = library->songs[current_index].artist;
    size_t found = 0;
    for (size_t i = 0; i < library->count && found < max_results; ++i) {
        if (i == current_index) {
            continue;
        }
        if (strcmp(library->songs[i].artist, artist) == 0) {
            output_indices[found++] = i;
        }
    }
    return found;
}
