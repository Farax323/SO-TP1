#ifndef MASTER_H
#define MASTER_H

#include "common.h"
#include <semaphore.h>
#include <stdbool.h>

#define MAX_PIPES 9

int **crear_pipes(int cantidad);
void cerrar_pipes_excepto(int **pipes, int cantidad, int idx, int extremo);
void colocar_jugadores(EstadoJuego *estado, unsigned int cantidad);
void inicializar_tablero(int *tablero, int width, int height, unsigned int seed);
bool mover_jugador(EstadoJuego *estado, int id, unsigned char dir);
bool jugador_esta_bloqueado(jugador *j, int *tablero, int width, int height);
bool todos_bloqueados(EstadoJuego *estado);

#endif // MASTER_H
