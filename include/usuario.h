#ifndef USUARIO_H
#define USUARIO_H
#include "commons.h"

typedef struct 
{
    tString username; // nombre del usuario
    long segundos_escucha_total; // segundos totales escuchados
    int reproducciones_totales; // número total de reproducciones
} tUsuario;

void usuario_print(const tUsuario* u);

typedef struct {
    int songId; // identificador de la canción
    int plays;  // número de reproducciones de la canción por el usuario
    long segundosEscuchados; // segundos escuchados de la canción por el usuario
} tUserSongStats;

#endif /* USUARIO_H */
