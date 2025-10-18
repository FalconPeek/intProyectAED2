#include "player.h"

#include <mpg123.h>
#include <portaudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define BUFFER_SIZE 8192

static int ensure_initialized(AudioPlayer *player) {
    if (player->initialized) {
        return 0;
    }
    return -1;
}

int player_init(AudioPlayer *player) {
    if (!player) {
        return -1;
    }
    memset(player, 0, sizeof(*player));

    int err = mpg123_init();
    if (err != MPG123_OK) {
        fprintf(stderr, "mpg123_init fallo: %s\n", mpg123_plain_strerror(err));
        return -1;
    }

    PaError pa_err = Pa_Initialize();
    if (pa_err != paNoError) {
        fprintf(stderr, "Pa_Initialize fallo: %s\n", Pa_GetErrorText(pa_err));
        mpg123_exit();
        return -1;
    }

    player->initialized = 1;
    return 0;
}

void player_shutdown(AudioPlayer *player) {
    if (!player || !player->initialized) {
        return;
    }
    Pa_Terminate();
    mpg123_exit();
    player->initialized = 0;
}

int player_play_song(AudioPlayer *player, const Song *song, const char *base_dir) {
    if (!song || !base_dir || ensure_initialized(player) != 0) {
        return -1;
    }

    char filepath[PATH_MAX];
#ifdef _WIN32
    snprintf(filepath, sizeof(filepath), "%s\\%s", base_dir, song->filename);
#else
    snprintf(filepath, sizeof(filepath), "%s/%s", base_dir, song->filename);
#endif

    mpg123_handle *mh = mpg123_new(NULL, NULL);
    if (!mh) {
        fprintf(stderr, "No se pudo crear el manejador mpg123.\n");
        return -1;
    }

    if (mpg123_open(mh, filepath) != MPG123_OK) {
        fprintf(stderr, "No se pudo abrir %s: %s\n", filepath, mpg123_strerror(mh));
        mpg123_delete(mh);
        return -1;
    }

    long rate;
    int channels;
    int encoding;
    if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
        fprintf(stderr, "No se pudo obtener el formato de %s\n", filepath);
        mpg123_close(mh);
        mpg123_delete(mh);
        return -1;
    }

    mpg123_format_none(mh);
    if (mpg123_format(mh, rate, channels, MPG123_ENC_SIGNED_16) != MPG123_OK) {
        fprintf(stderr, "Formato no soportado para %s\n", filepath);
        mpg123_close(mh);
        mpg123_delete(mh);
        return -1;
    }

    PaStream *stream;
    PaError pa_err = Pa_OpenDefaultStream(&stream, 0, channels, paInt16, (double)rate, paFramesPerBufferUnspecified, NULL, NULL);
    if (pa_err != paNoError) {
        fprintf(stderr, "No se pudo abrir el stream de audio: %s\n", Pa_GetErrorText(pa_err));
        mpg123_close(mh);
        mpg123_delete(mh);
        return -1;
    }

    pa_err = Pa_StartStream(stream);
    if (pa_err != paNoError) {
        fprintf(stderr, "No se pudo iniciar el stream: %s\n", Pa_GetErrorText(pa_err));
        Pa_CloseStream(stream);
        mpg123_close(mh);
        mpg123_delete(mh);
        return -1;
    }

    unsigned char buffer[BUFFER_SIZE];
    size_t done = 0;
    int mpg_err = MPG123_OK;

    printf("> Reproduciendo: %s — %s\n", song->title, song->artist);

    do {
        mpg_err = mpg123_read(mh, buffer, sizeof(buffer), &done);
        if (mpg_err != MPG123_OK && mpg_err != MPG123_DONE) {
            fprintf(stderr, "Error al decodificar %s: %s\n", filepath, mpg123_plain_strerror(mpg_err));
            break;
        }
        if (done == 0) {
            continue;
        }
        size_t frames = done / (sizeof(short) * (size_t)channels);
        if (frames == 0) {
            continue;
        }
        pa_err = Pa_WriteStream(stream, buffer, frames);
        if (pa_err != paNoError) {
            fprintf(stderr, "Error al escribir en el stream: %s\n", Pa_GetErrorText(pa_err));
            break;
        }
    } while (mpg_err != MPG123_DONE);

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    mpg123_close(mh);
    mpg123_delete(mh);

    if (mpg_err != MPG123_DONE) {
        fprintf(stderr, "La reproducción finalizó con código %d (%s)\n", mpg_err, mpg123_plain_strerror(mpg_err));
    } else {
        printf("> Canción finalizada\n");
    }

    return 0;
}
