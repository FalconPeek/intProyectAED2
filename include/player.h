#ifndef PLAYER_H
#define PLAYER_H

#include "library.h"

typedef struct {
    int initialized;
} AudioPlayer;

int player_init(AudioPlayer *player);
void player_shutdown(AudioPlayer *player);
int player_play_song(AudioPlayer *player, const Song *song, const char *base_dir);

#endif
