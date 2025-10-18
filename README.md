# Reproductor MP3 Minimalista

Aplicación de consola escrita en C99 que reproduce archivos MP3 ubicados en la
carpeta `canciones/`. El programa detecta automáticamente los nuevos archivos,
pide los metadatos básicos (título y artista) la primera vez que aparecen y
permite obtener recomendaciones simples basadas en el artista de la última
pista reproducida.

## Requisitos

- Compilador C99 (probado con `gcc 12`).
- [`libmpg123`](https://www.mpg123.de/) para decodificar MP3.
- [`PortAudio`](https://portaudio.com/) para la salida de audio.
- `make`.

En Linux (ej. Ubuntu) podés instalar las dependencias con:

```bash
sudo apt-get install libmpg123-dev portaudio19-dev
```

En Windows se recomienda compilar con [MSYS2](https://www.msys2.org/) o WSL:

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-portaudio mingw-w64-x86_64-mpg123
```

Asegurate de que los directorios de inclusión y librerías de PortAudio y
mpg123 estén disponibles en tu entorno antes de compilar.

## Compilación y ejecución

```bash
make            # compila en build/app
./build/app     # ejecuta la aplicación interactiva
```

Para recompilar desde cero:

```bash
make clean && make
```

## Flujo de uso

1. **Agregá archivos MP3** dentro del directorio `canciones/`.
2. **Ejecutá la aplicación** (`./build/app`).
3. La primera vez que se detecte una canción se solicitará el título y el
   artista. Esta información se guarda en `data/metadata.txt` para usos
   posteriores.
4. **Listá y reproducí canciones** desde el menú. La reproducción es real
   mediante libmpg123 + PortAudio.
5. **Consultá recomendaciones** para obtener más pistas del mismo artista que
   la canción reproducida recientemente.
6. **Reescanear** permite detectar nuevos archivos agregados en caliente.

## Estructura del proyecto

```
intProyectAED2/
├── canciones/           # Carpeta a la que debes copiar tus MP3
├── data/
│   └── metadata.txt     # Metadatos persistidos (se crea al vuelo)
├── include/             # Archivos de cabecera
├── src/                 # Código fuente
└── Makefile
```

## Notas

- El reproductor trabaja únicamente con archivos `.mp3` ubicados directamente
  bajo `canciones/`.
- Si necesitás limpiar los metadatos guardados, podés eliminar
  `data/metadata.txt`; la información será solicitada nuevamente en el próximo
  escaneo.
- No se mantienen estadísticas históricas ni estructuras de grafo: el enfoque
  es simple y directo para facilitar futuras ampliaciones.
