#ifndef USUARIO_H
#define USUARIO_H
#include "commons.h"

typedef struct 
{
    tString nombreUsuario; // nombre del usuario
    long segundosEscuchados; // segundos totales escuchados
    int reproduccionesTotales; // número total de reproducciones
} tUsuario;

void usuarioPrint(const tUsuario* u);

typedef struct {
    int songId; // identificador de la canción
    int plays;  // número de reproducciones de la canción por el usuario
    long segundosEscuchados; // segundos escuchados de la canción por el usuario
} tUserSongStats;
