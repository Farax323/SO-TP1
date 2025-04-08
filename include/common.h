#ifndef COMMON_H
#define COMMON_H

#include <semaphore.h>
#include <stdbool.h>

#define SHM_STATE "/game_state"
#define SHM_SYNC "/game_sync"
#define MAX_PLAYERS 9
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 10

typedef struct {
	char nombre[16];	  // nombre del jugador
	unsigned int puntaje; // puntaje del jugador
	unsigned int movs_invalidos;
	unsigned int movs_validos;
	unsigned short x, y; // posicion
	pid_t pid;			 // identificador del proceso
	bool bloqueado;		 // indica si el jugador tiene movimientos validos disponibles
} jugador;

typedef struct {
	unsigned short width;			 // ancho del tablero
	unsigned short height;			 // alto del tablero
	unsigned int cantidad_jugadores; // cantidad total de jugadores
	jugador jugadores[MAX_PLAYERS];	 // lista de jugadores
	bool juego_terminado;			 // indica si el juego termino
	int tablero[];					 // puntero al tablero de juego

} EstadoJuego;

typedef struct {
	sem_t sem_vista;	   // indica a la vista que hay cambios por imprimir
	sem_t sem_master;	   // indica al master que la vista termino de imprimir
	sem_t mutex_estado;	   // mutex para evitar inanicion del master al acceder al estado
	sem_t mutex_tablero;   // mutex para proteger el acceso al tablero
	sem_t mutex_lectores;  // mutex para la siguiente variable
	unsigned int lectores; // cantidad de jugadores leyendo el estado
} Sincronizacion;

#endif // COMMON_H