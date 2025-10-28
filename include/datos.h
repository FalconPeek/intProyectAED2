#define MAX_SIZE 50
#include "commons.h"

Person persona;

void pedirDatos() {
    printf("Inserte su nombre: ");
    fgets(persona.name, MAX_SIZE, stdin);
    printf("Inserte su edad: ");
    scanf("%d", &persona.age);
}

void mostrarDatos() {
    printf("Datos ingresados:\n");
    printf("Si quiere mostrar los datos aprete ENTER\n");
    getchar(); // Espera a que el usuario presione ENTER
    getchar(); // Captura el ENTER
    printf("Nombre: %s", persona.name);
    printf("Edad: %d\n", persona.age);
}