#include "../include/master.h"
#include <bits/getopt_core.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

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
    char *player_paths[9];
    int player_count = 0;

    int opt;
    while ((opt = getopt(argc, argv, "w:h:p:")) != -1) {
        switch (opt) {
        case 'w':
            width = atoi(optarg);
            break;
        case 'h':
            height = atoi(optarg);
            break;
        case 'p':
            if (player_count < 9) {
                player_paths[player_count++] = optarg;
            } else {
                fprintf(stderr, "Máximo 9 jugadores permitidos.\n");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            fprintf(stderr, "Uso: %s [-w width] [-h height] [-p player1 player2 ...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (player_count < 1) {
        fprintf(stderr, "Debe especificar al menos un jugador.\n");
        exit(EXIT_FAILURE);
    }

    size_t cant_celdas = width * height;
    size_t tam_total = sizeof(EstadoJuego) + cant_celdas * sizeof(int);

    // Crear SHM que contenga EstadoJuego + tablero
    void *mem_base = createSHM("/game_state", tam_total);
    EstadoJuego *estado = (EstadoJuego *)mem_base;

    // Inicializar estado
    estado->width = width;
    estado->height = height;
    estado->cantidad_jugadores = 0;
    estado->juego_terminado = false;

    // Puntero al tablero (parte final del bloque)
    // estado->tablero = (int *)(mem_base + sizeof(EstadoJuego));

    // Inicializar el tablero
    for (size_t i = 0; i < cant_celdas; i++) {
        estado->tablero[i] = (rand() % 9) + 1;
    }

    // printf("[MASTER] Tablero generado:\n");
    // for (int y = 0; y < estado->height; y++) {
    //     for (int x = 0; x < estado->width; x++) {
    //         printf("%d ", estado->tablero[y * estado->width + x]);
    //     }
    //     printf("\n");
    // }

    // Crear SHM de sincronización
    Sincronizacion *sync = (Sincronizacion *)createSHM("/game_sync", sizeof(Sincronizacion));
    sem_init(&sync->sem_vista, 1, 0);
    sem_init(&sync->sem_master, 1, 1);
    sem_init(&sync->mutex_estado, 1, 1);
    sem_init(&sync->mutex_tablero, 1, 1);
    sem_init(&sync->mutex_lectores, 1, 1);
    sync->lectores = 0;

    printf("Memoria compartida y semáforos inicializados correctamente.\n");

    // Crear procesos de los jugadores
    for (int i = 0; i < player_count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            fprintf(stderr, "Hola1\n");
            // Proceso hijo
            char width_str[10], height_str[10];
            sprintf(width_str, "%d", width);
            sprintf(height_str, "%d", height);
            execl(player_paths[i], "player", width_str, height_str, NULL);
        } else if (pid < 0) {
            perror("Error al crear proceso del jugador");
            exit(EXIT_FAILURE);
            printf("Hola2");
        }
    }

    return 0;
}
