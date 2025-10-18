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

#define COLOR_RESET "\033[0m"
#define COLOR_INFO "\033[38;5;45m"
#define COLOR_SUCCESS "\033[38;5;120m"
#define COLOR_ERROR "\033[38;5;203m"
#define COLOR_WARNING "\033[38;5;221m"
#define STYLE_BOLD "\033[1m"

static int ensure_initialized(AudioPlayer *player) {
    if (player->initialized) {
        return 0;
    }
    return -1;
}

static PaError open_output_stream(PaStream **stream, long rate, int channels) {
    if (!stream || channels <= 0) {
        return paInvalidChannelCount;
    }

    PaDeviceIndex default_device = Pa_GetDefaultOutputDevice();
    if (default_device != paNoDevice) {
        const PaDeviceInfo *info = Pa_GetDeviceInfo(default_device);
        if (info && info->maxOutputChannels >= channels) {
            PaStreamParameters params;
            params.device = default_device;
            params.channelCount = channels;
            params.sampleFormat = paInt16;
            params.suggestedLatency = info->defaultLowOutputLatency;
            params.hostApiSpecificStreamInfo = NULL;

            PaError err = Pa_OpenStream(stream, NULL, &params, (double)rate, paFramesPerBufferUnspecified, paNoFlag, NULL, NULL);
            if (err == paNoError) {
                const PaHostApiInfo *host_info = Pa_GetHostApiInfo(info->hostApi);
                const char *api_name = host_info ? host_info->name : "API desconocida";
                printf(COLOR_INFO STYLE_BOLD "> Dispositivo de salida:%s %s (%s)%s\n", COLOR_RESET, info->name, api_name, COLOR_RESET);
                return paNoError;
            }
            fprintf(stderr, "%sNo se pudo abrir el dispositivo de salida predeterminado: %s%s\n", COLOR_WARNING, Pa_GetErrorText(err), COLOR_RESET);
        }
    }

    PaDeviceIndex count = Pa_GetDeviceCount();
    if (count < 0) {
        return (PaError)count;
    }

    for (PaDeviceIndex i = 0; i < count; ++i) {
        if (i == default_device) {
            continue;
        }
        const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
        if (!info || info->maxOutputChannels < channels) {
            continue;
        }

        PaStreamParameters params;
        params.device = i;
        params.channelCount = channels;
        params.sampleFormat = paInt16;
        params.suggestedLatency = info->defaultLowOutputLatency;
        params.hostApiSpecificStreamInfo = NULL;

        PaError err = Pa_OpenStream(stream, NULL, &params, (double)rate, paFramesPerBufferUnspecified, paNoFlag, NULL, NULL);
        if (err == paNoError) {
            const PaHostApiInfo *host_info = Pa_GetHostApiInfo(info->hostApi);
            const char *api_name = host_info ? host_info->name : "API desconocida";
            printf(COLOR_INFO STYLE_BOLD "> Dispositivo alternativo:%s %s (%s)%s\n", COLOR_RESET, info->name, api_name, COLOR_RESET);
            return paNoError;
        }
    }

    return paDeviceUnavailable;
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

    PaStream *stream = NULL;
    PaError pa_err = open_output_stream(&stream, rate, channels);
    if (pa_err != paNoError) {
        fprintf(stderr, "%sNo se encontró un dispositivo de salida disponible: %s%s\n", COLOR_ERROR, Pa_GetErrorText(pa_err), COLOR_RESET);
        mpg123_close(mh);
        mpg123_delete(mh);
        return -1;
    }

    pa_err = Pa_StartStream(stream);
    if (pa_err != paNoError) {
        fprintf(stderr, "%sNo se pudo iniciar el stream: %s%s\n", COLOR_ERROR, Pa_GetErrorText(pa_err), COLOR_RESET);
        Pa_CloseStream(stream);
        mpg123_close(mh);
        mpg123_delete(mh);
        return -1;
    }

    unsigned char buffer[BUFFER_SIZE];
    size_t done = 0;
    int mpg_err = MPG123_OK;
    int success = 1;

    printf(COLOR_SUCCESS STYLE_BOLD "> Reproduciendo:%s %s — %s%s\n", COLOR_RESET, song->title, song->artist, COLOR_RESET);

    do {
        mpg_err = mpg123_read(mh, buffer, sizeof(buffer), &done);
        if (mpg_err != MPG123_OK && mpg_err != MPG123_DONE) {
            fprintf(stderr, "%sError al decodificar %s: %s%s\n", COLOR_ERROR, filepath, mpg123_plain_strerror(mpg_err), COLOR_RESET);
            success = 0;
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
            fprintf(stderr, "%sError al escribir en el stream: %s%s\n", COLOR_ERROR, Pa_GetErrorText(pa_err), COLOR_RESET);
            success = 0;
            break;
        }
    } while (mpg_err != MPG123_DONE);

    PaError stop_err = Pa_StopStream(stream);
    if (stop_err != paNoError) {
        fprintf(stderr, "%sNo se pudo detener el stream correctamente: %s%s\n", COLOR_WARNING, Pa_GetErrorText(stop_err), COLOR_RESET);
        success = 0;
    }
    Pa_CloseStream(stream);
    mpg123_close(mh);
    mpg123_delete(mh);

    if (success && mpg_err == MPG123_DONE) {
        printf(COLOR_SUCCESS STYLE_BOLD "> Canción finalizada%s\n", COLOR_RESET);
        return 0;
    }

    return -1;
}
