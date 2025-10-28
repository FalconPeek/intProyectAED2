#include <stdio.h>
#include "datos.h"
#include "commons.h"

/*
Como funciona el #ifdef? Respuesta: El #ifdef es una directiva de preprocesador en C
 que verifica si una macro está definida. Si la macro está definida, el código entre #ifdef 
 y #endif se incluye en la compilación. Si no está definida, se omite. Esto es útil para incluir o excluir
  partes del código según ciertas condiciones.
*/


int main(void) {
    pedirDatos();
    mostrarDatos();
    return 0;
}
