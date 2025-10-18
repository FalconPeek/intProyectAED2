#ifndef PERSIST_USUARIOS_TXT_H
#define PERSIST_USUARIOS_TXT_H

#include "usuarios_lista.h"

/* Persistencia en TXT "a la cátedra". 
   Formato (una línea por usuario, con separadores espacio):
   username segundos_escucha_total reproducciones_totales\n

   Ejemplo:
   lucas  1234  42
   emma   980   15
*/

/* Carga usuarios desde TXT. Si el archivo no existe, retorna 1 (colección vacía). */
int persist_usuarios_load_txt(const char* path, tListaU** L);

/* Guarda usuarios a TXT (sobrescribe). Retorna 1 si ok, 0 si error. */
int persist_usuarios_save_txt(const char* path, const tListaU* L);

#endif /* PERSIST_USUARIOS_TXT_H */
