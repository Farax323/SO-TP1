# ChompChamps - Sistema de Juego Multijugador

## Descripción General

ChompChamps es un juego multijugador desarrollado como parte del TP1 para Sistemas Operativos. Los jugadores compiten en un tablero rectangular recolectando recompensas mientras navegan en 8 direcciones posibles. El sistema está implementado usando conceptos avanzados de IPC (Inter-Process Communication) como memoria compartida, semáforos y pipes.

## Arquitectura del Sistema

El sistema está dividido en tres componentes principales:

1. **Proceso Master** (`master.c`): Coordina el juego, gestiona el estado global, inicializa el tablero y maneja la comunicación entre procesos.

2. **Procesos Jugadores** (`player.c`): Cada jugador es un proceso independiente que toma decisiones sobre movimientos basados en una estrategia simple.

3. **Proceso Vista** (`view.c`): Visualiza el estado del juego en tiempo real mediante una interfaz en consola con colores.

## Detalles Técnicos

- **Memoria Compartida**: Implementada con `shm_open` para compartir el tablero y el estado del juego entre procesos.
- **Sincronización**: Utiliza semáforos POSIX para prevenir condiciones de carrera.
- **Comunicación**: Mediante pipes unidireccionales entre el master y los jugadores.
- **Representación del Tablero**: Matriz de enteros donde valores positivos son recompensas y valores negativos representan jugadores.

## Requisitos

- Sistema operativo Linux (probado en distribuciones basadas en Debian)
- Compilador GCC
- Biblioteca de hilos POSIX (pthread)
- Biblioteca de tiempo real (rt)

## Compilación

El proyecto incluye un Makefile para facilitar la compilación:

```bash
# Compilar todos los componentes
make

# Formatear el código fuente
make format

# Limpiar binarios
make clean
```

## Ejecución

**IMPORTANTE:** La carpeta `bin/` está vacía en Git, pero es necesaria. Se ha agregado un archivo `.gitkeep` para que se mantenga en el repositorio.

Para iniciar el juego:

```bash
# Formato básico
./bin/master [opciones]

# Ejemplo con un jugador
./bin/master -p bin/player

# Ejemplo completo con visualización
./bin/master -w 15 -h 15 -d 100 -t 20 -s 42 -v bin/view -p bin/player bin/player bin/player
```

### Opciones de Configuración

| Opción | Descripción | Valor predeterminado |
|--------|-------------|----------------------|
| `-w` | Ancho del tablero | 10 |
| `-h` | Altura del tablero | 10 |
| `-d` | Retardo entre movimientos (ms) | 200 |
| `-t` | Tiempo límite de juego (s) | 10 |
| `-s` | Semilla aleatoria | Timestamp actual |
| `-v` | Ruta al ejecutable de visualización | No se muestra visualización |
| `-p` | Rutas a los ejecutables de jugadores | Ningún jugador |

## Cómo Funciona el Juego

1. **Inicio**: El master inicializa el tablero con recompensas aleatorias (valores 1-9) y coloca a los jugadores.
2. **Movimientos**: Los jugadores envían comandos de dirección (números 0-7) para moverse por el tablero.
3. **Puntuación**: Cuando un jugador se mueve a una celda con recompensa, suma esos puntos a su total.
4. **Finalización**: El juego termina cuando se alcanza el timeout o todos los jugadores quedan bloqueados.
5. **Resultado**: Se muestra un ranking final basado en los puntos acumulados.

## Estructura del Código

- **include**: Archivos de cabecera con definiciones y estructuras
  - `common.h`: Estructuras de datos compartidas
  - `shm.h`: Funciones para memoria compartida
  - `master.h`: Funciones del proceso master
  - `player.h`: Funciones del proceso jugador
  - `view.h`: Funciones del proceso vista

- **src**: Implementaciones de los componentes
  - `master.c`: Proceso principal coordinador
  - `player.c`: Algoritmo de toma de decisiones del jugador
  - `shm.c`: Implementación de memoria compartida
  - `view.c`: Visualización del estado del juego
