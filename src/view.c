#include "../include/master.h"
#include <asm-generic/fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define CLEAR_SCREEN "\033[2J\033[H"
#define COLOR_RESET "\033[0m"
#define COLOR_REWARD_LOW "\033[38;5;152m" // Azul pastel
#define COLOR_REWARD_MEDIUM "\033[38;5;222m" // Amarillo pastel
#define COLOR_REWARD_HIGH "\033[38;5;210m" // Rojo/coral pastel
#define COLOR_BLOCKED "\033[1;90m"

void imprimir_tablero(EstadoJuego *estado, int *tablero) {
    const char *colores[] = {
        "\033[38;5;160m", // rojo
        "\033[38;5;46m", // verde
        "\033[38;5;226m", // amarillo
        "\033[38;5;39m", // azul
        "\033[38;5;201m", // magenta
        "\033[38;5;51m", // cian
        "\033[38;5;208m", // naranja
        "\033[38;5;129m", // violeta
        "\033[38;5;27m" // celeste
    };
    size_t cantidad_colores = sizeof(colores) / sizeof(colores[0]);

    fprintf(stderr, "    ");
    for (int x = 0; x < estado->width; x++) {
        fprintf(stderr, " %2d", x);
    }
    fprintf(stderr, "\n   +");
    for (int x = 0; x < estado->width; x++) {
        fprintf(stderr, "---");
    }
    fprintf(stderr, "+\n");

    for (int y = 0; y < estado->height; y++) {
        fprintf(stderr, "%2d |", y);
        for (int x = 0; x < estado->width; x++) {
            int val = estado->tablero[y * estado->width + x];

            if (val >= 1 && val <= 9) {
                const char *color = val <= 3 ? COLOR_REWARD_LOW : val <= 6 ? COLOR_REWARD_MEDIUM
                                                                           : COLOR_REWARD_HIGH;
                fprintf(stderr, "%s %2d%s", color, val, COLOR_RESET);
            } else if (val < 0) {
                int id = (-val) - 1;
                if (id >= 0 && id < estado->cantidad_jugadores) {
                    const char *color = (id < cantidad_colores) ? colores[id] : COLOR_RESET;
                    jugador *j = &estado->jugadores[id];
                    char label[4];
                    if (j->x == x && j->y == y) {
                        snprintf(label, sizeof(label), "@%d", id);
                    } else {
                        snprintf(label, sizeof(label), "P%d", id);
                    }
                    fprintf(stderr, "%s%3s%s", color, label, COLOR_RESET);
                } else {
                    fprintf(stderr, " ??");
                }
            } else {
                fprintf(stderr, " ??");
            }
        }
        fprintf(stderr, " |\n");
    }

    fprintf(stderr, "   +");
    for (int x = 0; x < estado->width; x++) {
        fprintf(stderr, "---");
    }
    fprintf(stderr, "+\n");
}

void imprimir_ranking(EstadoJuego *estado) {
    typedef struct {
        int id;
        unsigned int puntaje;
        unsigned int validos;
        unsigned int invalidos;
    } Ranking;

    Ranking ranking[MAX_PLAYERS];
    for (unsigned int i = 0; i < estado->cantidad_jugadores; i++) {
        ranking[i].id = i;
        ranking[i].puntaje = estado->jugadores[i].puntaje;
        ranking[i].validos = estado->jugadores[i].movs_validos;
        ranking[i].invalidos = estado->jugadores[i].movs_invalidos;
    }

    for (unsigned int i = 0; i < estado->cantidad_jugadores; i++) {
        for (unsigned int j = i + 1; j < estado->cantidad_jugadores; j++) {
            bool menor = false;
            if (ranking[j].puntaje > ranking[i].puntaje)
                menor = true;
            else if (ranking[j].puntaje == ranking[i].puntaje && ranking[j].validos < ranking[i].validos)
                menor = true;
            else if (ranking[j].puntaje == ranking[i].puntaje && ranking[j].validos == ranking[i].validos && ranking[j].invalidos < ranking[i].invalidos)
                menor = true;

            if (menor) {
                Ranking tmp = ranking[i];
                ranking[i] = ranking[j];
                ranking[j] = tmp;
            }
        }
    }

    fprintf(stderr, "\n========== RANKING =========\n");
    for (unsigned int i = 0; i < estado->cantidad_jugadores; i++) {
        jugador *j = &estado->jugadores[ranking[i].id];
        const char *medalla = (i == 0) ? " [Líder]" : "";

        fprintf(stderr, "%d\t%-15s Puntaje: %3u | V: %3u | I: %3u%s\n",
            ranking[i].id,
            j->nombre[0] ? j->nombre : "(sin nombre)",
            ranking[i].puntaje, ranking[i].validos, ranking[i].invalidos, medalla);
    }
    fprintf(stderr, "============================\n");
}

void imprimir_jugadores(EstadoJuego *estado) {
    fprintf(stderr, "\n========== ESTADO DE JUGADORES =========\n");
    for (unsigned int i = 0; i < estado->cantidad_jugadores; i++) {
        jugador *j = &estado->jugadores[i];
        const char *color_estado = j->bloqueado ? COLOR_BLOCKED : "";
        fprintf(stderr, "%sJugador %d: %-15s [%s]%s\n", color_estado, i,
            j->nombre[0] ? j->nombre : "(sin nombre)",
            j->bloqueado ? "Bloqueado" : "Activo", COLOR_RESET);
        fprintf(stderr, "%s  Posición : (%2hu, %2hu)\n", color_estado, j->x, j->y);
        fprintf(stderr, "%s  Puntaje  : %3u\n", color_estado, j->puntaje);
        fprintf(stderr, "%s  Movs     : Válidos: %3u | Inválidos: %3u%s\n\n",
            color_estado, j->movs_validos, j->movs_invalidos, COLOR_RESET);
    }
    fprintf(stderr, "========================================\n");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <width> <height>\n", argv[0]);
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

    time_t start_time = time(NULL);
    int frame_count = 0;

    while (!estado->juego_terminado) {
        sem_wait(&sync->sem_vista);

        fprintf(stderr, "\033[H\033[J"); // Mover cursor y limpiar pantalla
        fprintf(stderr, "\033[3J"); // Borrar scrollback (extra)
        fflush(stderr);

        fprintf(stderr, "======= ChompChamps G15 - Vista del Juego =======\n");

        imprimir_tablero(estado, tablero);
        imprimir_jugadores(estado);
        imprimir_ranking(estado);

        time_t now = time(NULL);
        fprintf(stderr, "\nTiempo transcurrido: %ld segundos\n", now - start_time);
        fprintf(stderr, "Frames renderizados : %d\n", ++frame_count);

        sem_post(&sync->sem_master);
        usleep(100000);
    }

    fprintf(stderr, "\n[VIEW] Juego terminado.\n");

    // Estadísticas finales
    fprintf(stderr, "\n======= ESTADÍSTICAS FINALES =======\n");
    imprimir_ranking(estado);
    fprintf(stderr, "Duración total       : %ld segundos\n", time(NULL) - start_time);
    fprintf(stderr, "Frames totales       : %d\n", frame_count);
    fprintf(stderr, "====================================\n");

    return 0;
}
