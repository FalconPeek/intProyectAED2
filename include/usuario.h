#ifndef USUARIO_H
#define USUARIO_H

#include "commons.h"

/* Modelo estándar usado en persist.c */
typedef struct {
    tString username;                 /* clave primaria (sin espacios) */
    long    segundos_escucha_total;   /* acumulado en segundos */
    int     reproducciones_totales;   /* cantidad total de reproducciones */
} tUsuario;

/* Utilidad para imprimir (debug/reportes) */
void usuario_print(const tUsuario* u);

#endif /* USUARIO_H */
