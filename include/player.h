#ifndef PLAYER_H
#define PLAYER_H

#include "common.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

int direccion_hacia(int x0, int y0, int x1, int y1);
int distancia(int x0, int y0, int x1, int y1);
int jugadores_cerca(int x, int y, EstadoJuego *estado, int mi_id);
EstadoJuego *inicializar_estado(char *path, size_t tam_total,Sincronizacion *sync);
Sincronizacion *inicializar_sincronizacion(char *path);
jugador *encontrar_jugador(EstadoJuego *estado, pid_t mi_pid);
void calcular_mejor_movimiento(EstadoJuego *estado, jugador *yo, int *mejor_x, int *mejor_y);
void procesar_juego(EstadoJuego *estado, jugador *yo);

#endif // PLAYER_H