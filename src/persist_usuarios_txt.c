#include "persist_usuarios_txt.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* Carga:
   - Abre TXT en modo lectura ("r").
   - Lee la PRIMERA línea con fscanf (lectura adelantada).
   - Si la lectura fue exitosa, procesa/insertar; luego vuelve a leer para el siguiente.
   - Cortar al llegar a EOF (usando !feof y validando fscanf).
   - Si el archivo no existe, consideramos OK (colección vacía). */
int persist_usuarios_load_txt(const char* path, tListaU** L){
    FILE* f = fopen(path, "r");
    if(!f){
        /* Si no existe, para la práctica lo consideramos "ok, vacío". */
        return (errno == ENOENT) ? 1 : 0;
    }

    /* buffers temporales para lectura */
    tUsuario u;
    /* Lectura adelantada:
       - leemos primero y luego entramos al while (!feof)
       - en fscanf, %49s para string (sin espacios) y %ld / %d para números */
    int r = fscanf(f, " %49s %ld %d",
                   u.username, &u.segundos_escucha_total, &u.reproducciones_totales);

    while(!feof(f)){
        if(r == 3){
            if(!listaU_push_front(L, &u)){
                fclose(f);
                return 0; /* sin memoria */
            }
        }
        /* volver a leer para siguiente iteración */
        r = fscanf(f, " %49s %ld %d",
                   u.username, &u.segundos_escucha_total, &u.reproducciones_totales);
    }

    fclose(f);
    return 1;
}

/* Guarda:
   - Abre en "w" (sobrescribe).
   - Recorre lista y escribe cada usuario en una línea.
   - Cierra y retorna 1 si ok. */
int persist_usuarios_save_txt(const char* path, const tListaU* L){
    FILE* f = fopen(path, "w");
    if(!f) return 0;

    for(; L; L = L->sig){
        /* username sin espacios; si necesitas permitirlos, usa "%[^\n]" y otro formato de línea. */
        int n = fprintf(f, "%s %ld %d\n",
                        L->info.username,
                        L->info.segundos_escucha_total,
                        L->info.reproducciones_totales);
        if(n <= 0){ fclose(f); return 0; }
    }
    fclose(f);
    return 1;
}
