#include <stdio.h>
#include "commons.h"

/* Fase 01
    - Mostrar banner del proyecto
    - No usar aun otros modulos
*/

static void print_banner() {
    printf("=====================================\n");
    printf("      Proyecto Integrador en C       \n");
    printf("           AdivinaNro+               \n");
    printf("          Version 1.0.0              \n");
    printf("       Por Lucas y Alexis            \n");    
    printf("=====================================\n");
}

int main() {
    print_banner();
    printf("MAX_STR = %d\n", MAX_STR);
    puts("Build OK. Proxima Fase: TADs base (RNG, lista, pila, cola, BST).");
    return OK;
}