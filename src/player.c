#include "../include/master.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

int dx[] = { 0, 1, 1, 1, 0, -1, -1, -1 };
int dy[] = {-1, -1, 0, 1, 1,  1,  0, -1 };

int direccion_hacia(int x0, int y0, int x1, int y1) {
    int dx_ = x1 - x0;
    int dy_ = y1 - y0;
    if (dx_ > 0) dx_ = 1;
    else if (dx_ < 0) dx_ = -1;
    if (dy_ > 0) dy_ = 1;
    else if (dy_ < 0) dy_ = -1;
    for (int dir = 0; dir < 8; dir++) {
        if (dx[dir] == dx_ && dy[dir] == dy_) return dir;
    }
    return -1;
}

int distancia(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    return dx > dy ? dx : dy;
}

int jugadores_cerca(int x, int y, EstadoJuego *estado, int mi_id) {
    int count = 0;
    for (unsigned int i = 0; i < estado->cantidad_jugadores; i++) {
        if ((int)i == mi_id) continue;
        jugador *j = &estado->jugadores[i];
        if (abs(j->x - x) <= 1 && abs(j->y - y) <= 1) count++;
    }
    return count;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <width> <height>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    srand(getpid() ^ time(NULL));

    int shm_fd = shm_open("/game_state", O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("Jugador: Error al abrir memoria compartida");
        exit(EXIT_FAILURE);
    }

    size_t tam_total = sizeof(EstadoJuego) + width * height * sizeof(int);
    void *mem_base = mmap(NULL, tam_total, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (mem_base == MAP_FAILED) {
        perror("Jugador: Error al mapear memoria");
        exit(EXIT_FAILURE);
    }

    EstadoJuego *estado = (EstadoJuego *)mem_base;
    int *tablero = (int *)((char *)estado + sizeof(EstadoJuego));

    pid_t mi_pid = getpid();
    int mi_id = -1;
    jugador *yo = NULL;
    for (unsigned int i = 0; i < estado->cantidad_jugadores; i++) {
        if (estado->jugadores[i].pid == mi_pid) {
            mi_id = i;
            yo = &estado->jugadores[i];
            break;
        }
    }

    if (!yo) {
        fprintf(stderr, "Jugador: No encontrado en el estado.\n");
        exit(EXIT_FAILURE);
    }

    while (!estado->juego_terminado && !yo->bloqueado) {
        int mejor_x = -1, mejor_y = -1;
        float mejor_score = -1.0f;

        for (int y = 0; y < estado->height; y++) {
            for (int x = 0; x < estado->width; x++) {
                int val = tablero[y * estado->width + x];
                if (val <= 0) continue;

                int dist = distancia(yo->x, yo->y, x, y);
                if (dist == 0 || dist > 1) continue; // solo considerar celdas adyacentes

                int rivales = jugadores_cerca(x, y, estado, mi_id);
                float score = val / (float)(dist + 1 + rivales);

                if (score > mejor_score) {
                    mejor_score = score;
                    mejor_x = x;
                    mejor_y = y;
                }
            }
        }

        if (mejor_x != -1 && mejor_y != -1) {
            int dir = direccion_hacia(yo->x, yo->y, mejor_x, mejor_y);
            if (dir != -1) {
                unsigned char direccion = (unsigned char)dir;
                write(STDOUT_FILENO, &direccion, sizeof(direccion));
                sleep(1);
                continue;
            }
        }

        sleep(1);
    }

    return 0;
}