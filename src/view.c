#include "../include/master.h"
#include <asm-generic/fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

// "/game_state"
// "/game_sync"

void imprimir_tablero(EstadoJuego *estado, int *tablero) {
    const char *colores[] = {
        "\033[1;31m", "\033[1;32m", "\033[1;33m",
        "\033[1;34m", "\033[1;35m", "\033[1;36m",
        "\033[1;91m", "\033[1;92m", "\033[1;94m"
    };
    const char *reset_color = "\033[0m";

    fprintf(stderr, "\n--- TABLERO (%dx%d) ---\n", estado->width, estado->height);

    for (int y = 0; y < estado->height; y++) {
        for (int x = 0; x < estado->width; x++) {
            int val = tablero[y * estado->width + x];

            
            // if (val >= 1 && val <= 9) {
                // Celda libre con recompensa
                fprintf(stderr, " %d ", val);
            // }
            // } else if (val <= 0) {
            //     int id = (val == 0) ? 0 : -val;

            //     if (id >= 0 && id < estado->cantidad_jugadores) {
            //         const char *color = colores[id % 9];
            //         jugador *j = &estado->jugadores[id];

            //         if (j->x == x && j->y == y) {
            //             fprintf(stderr, "%s@%d%s ", color, id, reset_color); // Cabeza
            //         } else {
            //             fprintf(stderr, "%sP%d%s ", color, id, reset_color); // Cuerpo
            //         }
            //     } else {
            //         fprintf(stderr, " ? ");
            //     }
            // } else {
            //     fprintf(stderr, " ? ");
            // }
        }
        fprintf(stderr, "\n");
    }
}




void imprimir_jugadores(EstadoJuego *estado) {

    fprintf(stderr, "\n--- JUGADORES (%d) ---\n", estado->cantidad_jugadores);
    for (unsigned int i = 0; i < estado->cantidad_jugadores; i++) {
        jugador *j = &estado->jugadores[i];
        fprintf(stderr, "Jugador %d: %s\n", i, j->nombre[0] ? j->nombre : "(sin nombre)");
        fprintf(stderr, " - Posicion: (%hu, %hu)\n", j->x, j->y);
        fprintf(stderr, " - Puntaje: %u\n", j->puntaje);
        fprintf(stderr, " - Movs válidos: %u | inválidos: %u\n", j->movs_validos, j->movs_invalidos);
        fprintf(stderr, " - %s\n", j->bloqueado ? "Bloqueado" : "Puede moverse");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <width> <heigth>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    size_t tam_total = sizeof(EstadoJuego) + width * height * sizeof(int);

    int fd_estado = shm_open("/game_state", O_RDONLY, 0666);

    if (fd_estado == -1) {
        perror("VIEW: Error al abrir /game_state");
        return EXIT_FAILURE;
    }

    void *mem_base = mmap(NULL, tam_total, PROT_READ, MAP_SHARED, fd_estado, 0);

    if (mem_base == MAP_FAILED) {
        perror("VIEW: Error al mapear /game_state");
        return EXIT_FAILURE;
    }
    EstadoJuego *estado = (EstadoJuego *)mem_base;
    int *tablero = (int *)((char *)estado + sizeof(EstadoJuego));

    int fd_sync = shm_open("/game_sync", O_RDWR, 0666);

    if (fd_sync == -1) {
        perror("VIEW: Error al abrir /game_sync");
        return EXIT_FAILURE;
    }

    Sincronizacion *sync = mmap(NULL, sizeof(Sincronizacion), PROT_READ | PROT_WRITE, MAP_SHARED, fd_sync, 0);
    if (sync == MAP_FAILED) {
        perror("VIEW: Error al mapear /game_sync");
        return EXIT_FAILURE;
    }

    while (!estado->juego_terminado) {
        sem_wait(&sync->sem_vista); // Espera a que el máster indique que hay algo nuevo

        // Imprimir estado
        imprimir_tablero(estado, tablero);
        imprimir_jugadores(estado);

        sem_post(&sync->sem_master); // Indica que terminó de imprimir
        usleep(100000); // opcional, para que no explote la salida visual
    }

    fprintf(stderr, "\n[VIEW] Juego terminado.\n");
    return 0;
}
