# Reproductor & Recomendador Offline (AED2)

Aplicación de consola escrita en C99 que integra múltiples estructuras de datos
(lista, cola, pila, BST, trie, grafo) para gestionar una biblioteca musical,
construir playlists y ofrecer recomendaciones básicas. El programa soporta
múltiples usuarios, estadísticas detalladas por canción y persistencia en
archivos de texto.

## Características principales

- **Biblioteca musical:** carga/guardado desde `data/biblioteca.txt`, índice BST
  por artista+título y trie para autocompletado por prefijo.
- **Playlists persistentes:** lista enlazada con undo mediante pila, búsqueda
  lineal y binaria (con métricas de comparaciones) y guardado automático en
  `data/playlist/miplaylist.txt`.
- **Reproducción y cola:** cola FIFO para la lista de reproducción activa.
  Incrementa el `playcount` de la biblioteca y actualiza estadísticas del
  usuario logueado.
- **Recomendaciones:** grafo de similitud construido por artista (optimizado a
  O(n log n) mediante ordenamiento), con BFS para sugerir canciones.
- **Usuarios y estadísticas:** persistidos en `data/usuarios.txt`, con métricas
  agregadas (reproducciones y segundos). Las estadísticas por canción se
  almacenan por usuario en `data/userstats/<usuario>.txt` y alimentan el
  leaderboard y los tops personales.
- **Persistencia automática:** directorios `data/playlist/` y `data/userstats/`
  se crean al inicio; al salir se guardan los cambios pendientes.

## Requisitos y compilación

- Compilador C99 (probado con `gcc 12`).
- `make`.

```bash
make            # compila en build/app
./build/app     # ejecuta la aplicación interactiva
```

Para recompilar desde cero:

```bash
make clean && make
```

## Uso rápido

1. **Cargar biblioteca** desde el menú (opción 1).
2. **Login/crear usuario** (opción 2). Las estadísticas por canción se cargan
   automáticamente.
3. **Explorar** la biblioteca con búsquedas BST o trie (opciones 3 y 4).
4. **Gestionar la playlist** (opción 5): agregar/quitar canciones, buscar por ID
   o título (lineal/binaria) y deshacer acciones.
5. **Reproducir canciones** (opción 6) para actualizar métricas y reproducir la
   cola.
6. **Recomendaciones** (opción 8) y **estadísticas/leaderboards** (opción 9).
7. **Guardar** en cualquier momento (opción 11) o dejar que el cierre guarde
   automáticamente si hubo cambios.

## Datos y estructura de directorios

```
intProyectAED2/
├── data/
│   ├── biblioteca.txt           # Biblioteca musical
│   ├── playlist/miplaylist.txt  # Playlist persistida
│   ├── usuarios.txt             # Métricas agregadas por usuario
│   └── userstats/               # Directorio autogenerado para stats por usuario
└── build/                       # Binarios generados por make
```

## Verificación rápida / debug manual

La aplicación es interactiva. Para revisar el flujo completo se recomienda:

1. Compilar (`make`).
2. Ejecutar `./build/app` y recorrer el menú, verificando carga, búsqueda,
   reproducción, undo y guardado.
3. Revisar los archivos en `data/` tras guardar o salir para confirmar la
   persistencia.

Además se puede usar `valgrind` (`valgrind ./build/app`) para detectar fugas de
memoria durante sesiones de depuración.

