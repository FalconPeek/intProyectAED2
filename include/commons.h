#ifndef COMMONS_H
#define COMMONS_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_STR 50

typedef enum {
    OK = 0,
    ERROR = -1
} tStatus;

typedef char tString[MAX_STR]; // Cadena de caracteres de longitud máxima 49 + '\0'

#endif // COMMONS_H