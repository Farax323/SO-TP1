#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "../include/master.h"

#define SHM_ESTADO "/estado"

int main() {
    int shm_fd = shm_open(SHM_ESTADO, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("jugador: Error al abrir /estado");
        exit(EXIT_FAILURE);
    }

    // Mapear temporalmente para leer dimensiones
    EstadoJuego *tmp = (EstadoJuego *) mmap(NULL, sizeof(EstadoJuego), PROT_READ, MAP_SHARED, shm_fd, 0);
    if (tmp == MAP_FAILED) {
        perror("Jugador: Error al mapear estado temporalmente");
        exit(EXIT_FAILURE);
    }
    int width = tmp->width;
    int height = tmp->height;
    
    size_t tam_total = sizeof(EstadoJuego) + width * height * sizeof(int);
    munmap(tmp, sizeof(EstadoJuego));  // Liberar mapeo temporal

    // Mapear memoria completa
    void *mem_base = mmap(NULL, tam_total, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (mem_base == MAP_FAILED) {
        perror("Jugador: Error al mapear memoria completa");
        exit(EXIT_FAILURE);
    }
    EstadoJuego *estado = (EstadoJuego *)mem_base;

    // Reconstruir puntero al tablero
    estado->tablero = (int *)((char *)mem_base + sizeof(EstadoJuego));
    
    printf("Tablero generado:\n");
    for (int y = 0; y < estado->height; y++) {
        for (int x = 0; x < estado->width; x++) {
            printf("%d ", estado->tablero[y * estado->width + x]);
        }
        printf("\n");
    }

    return 0;
}

