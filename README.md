# Sistema de Juego Multijugador - ChompChamps

# Descripción

Este proyecto implementa un sistema de juego multijugador para el juego ChompChamps. El juego se ejecuta en un entorno de múltiples procesos utilizando memoria compartida, semáforos y pipes para la comunicación entre procesos. Cada jugador es un proceso independiente que interactúa en un tablero de juego compartido. El objetivo es implementar una sincronización efectiva entre los procesos para asegurar la integridad de los datos y proporcionar una experiencia de juego fluida y sin errores.

# Características
- Uso de memoria compartida para el estado del juego.

- Comunicación entre procesos con pipes.

- Sincronización con semáforos para prevenir condiciones de carrera.

- Proceso de vista para mostrar el estado del juego en tiempo real.
  
# Compilación y Ejecución

Para compilar el proyecto, navega al directorio raíz del proyecto y ejecuta el comando make:

make

Esto compilará todos los módulos necesarios y generará el ejecutable master.o . Para iniciar el juego, puedes usar el siguiente comando:

./master.o  -p player.o

Esto iniciara el juego con un jugador y con los elementos default (tablero 10x10), para personalisar el juego se pueden usar las distintas flags para modificar los valores

El proyecto necesita la siguientes carpetas para funcionar correctamente:
- `bin/` → Carpeta donde se generarán los ejecutables después de compilar.
-   - ⚠ **IMPORTANTE:** La carpeta `bin/` está vacía en Git, pero es necesaria. Se ha agregado un archivo `.gitkeep` para que se mantenga en el repositorio.


Para compilar y ejecutar el proyecto:
make
