#ifndef PERSIST_USUARIOS_TXT_H
#define PERSIST_USUARIOS_TXT_H
#include "usuarios_lista.h"

/* username segundos_escucha_total reproducciones_totales */
int persist_usuarios_load_txt(const char* path, tListaU** L);
int persist_usuarios_save_txt(const char* path, const tListaU* L);

#endif
