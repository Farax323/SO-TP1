#include "../include/master.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <width> <height>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    fprintf(stderr, "[PLAYER] width: %d, height: %d\n", width, height);

    srand(getpid() ^ time(NULL));

    // Open shared memory with more explicit error handling
    int shm_fd = shm_open("/game_state", O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("Jugador: Error al abrir memoria compartida");
        exit(EXIT_FAILURE);
    }

    size_t tam_total = sizeof(EstadoJuego) + width * height * sizeof(int);

    // Mapear memoria completa
    void *mem_base = mmap(NULL, tam_total, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (mem_base == MAP_FAILED) {
        perror("Jugador: Error al mapear memoria completa");
        exit(EXIT_FAILURE);
    }

    EstadoJuego *estado = (EstadoJuego *)mem_base;
    int *tablero = (int *)((char *)estado + sizeof(EstadoJuego));

    for (int i = 0; i < 1; i++) {
        unsigned char direccion = 2;//rand() % 8;
        write(STDOUT_FILENO, &direccion, sizeof(direccion));
        sleep(2);
    }
    return 0;
}