#ifndef VIEW_H
#define VIEW_H

#include "common.h"
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
#define COLOR_REWARD_LOW "\033[38;5;152m"
#define COLOR_REWARD_MEDIUM "\033[38;5;222m"
#define COLOR_REWARD_HIGH "\033[38;5;210m"
#define COLOR_BLOCKED "\033[1;90m"

void imprimir_tablero(EstadoJuego *estado);
void imprimir_ranking(EstadoJuego *estado);
void imprimir_jugadores(EstadoJuego *estado);
EstadoJuego *inicializar_estado(char *path, size_t tam_total);
Sincronizacion *inicializar_sincronizacion(char *path);
void procesar_juego(EstadoJuego *estado, Sincronizacion *sync, time_t *start_time, int *frame_count);

#endif // VIEW_H