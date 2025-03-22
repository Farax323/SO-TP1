#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../include/master.h"
#include <fcntl.h>
#include <semaphore.h>
#include <bits/getopt_core.h>

#define SHM_ESTADO "/estado"
#define SHM_SYNC "/sync"

void *createSHM(char *name, size_t size) {
    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("Error al crear memoria compartida");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, size) == -1) {
        perror("Error al truncar memoria compartida");
        exit(EXIT_FAILURE);
    }

    void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("Error al mapear memoria compartida");
        exit(EXIT_FAILURE);
    }

    return p;
}

int main(int argc, char *argv[]) {
    int width = 10;
    int height = 10;

    int opt;
    while ((opt = getopt(argc, argv, "w:h:")) != -1) {
        switch (opt) {
            case 'w': width = atoi(optarg); break;
            case 'h': height = atoi(optarg); break;
            default:
                fprintf(stderr, "Uso: %s [-w width] [-h height]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    size_t cant_celdas = width * height;
    size_t tam_total = sizeof(EstadoJuego) + cant_celdas * sizeof(int);

    // Crear SHM que contenga EstadoJuego + tablero
    void *mem_base = createSHM("/estado", tam_total);
    EstadoJuego *estado = (EstadoJuego *)mem_base;

    // Inicializar estado
    estado->width = width;
    estado->height = height;
    estado->cantidad_jugadores = 0;
    estado->juego_terminado = false;

    // Puntero al tablero (parte final del bloque)
    estado->tablero = (int *)(mem_base + sizeof(EstadoJuego));

    // Inicializar el tablero
    for (int i = 0; i < cant_celdas; i++) {
        estado->tablero[i] = (rand() % 9) + 1;
    }

    printf("Tablero generado:\n");
    for (int y = 0; y < estado->height; y++) {
        for (int x = 0; x < estado->width; x++) {
            printf("%d ", estado->tablero[y * estado->width + x]);
        }
        printf("\n");
    }

    // Crear SHM de sincronización
    Sincronizacion *sync = (Sincronizacion *)createSHM("/sync", sizeof(Sincronizacion));
    sem_init(&sync->sem_vista, 1, 0);
    sem_init(&sync->sem_master, 1, 1);
    sem_init(&sync->mutex_estado, 1, 1);
    sem_init(&sync->mutex_tablero, 1, 1);
    sem_init(&sync->mutex_lectores, 1, 1);
    sync->lectores = 0;

    printf("Memoria compartida y semáforos inicializados correctamente.\n");

    return 0;
}

