#ifndef LIBRARY_H
#define LIBRARY_H

#include <stddef.h>

#define LIBRARY_MAX_FIELD 128

typedef struct {
    char filename[512];
    char title[LIBRARY_MAX_FIELD];
    char artist[LIBRARY_MAX_FIELD];
} Song;

typedef struct {
    Song *songs;
    size_t count;
    size_t capacity;
    char songs_dir[512];
    char metadata_path[512];
} SongLibrary;

int library_init(SongLibrary *library, const char *songs_dir, const char *metadata_path);
void library_free(SongLibrary *library);
int library_scan(SongLibrary *library);
void library_print(const SongLibrary *library, size_t highlight_index);

size_t library_recommend_by_artist(const SongLibrary *library, size_t current_index, size_t *output_indices, size_t max_results);

#endif
