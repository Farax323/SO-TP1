#ifndef MASTER_H
#define MASTER_H

#include <semaphore.h>
#include <stdbool.h>



#define MAX_PLAYERS 9
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 10

//struct Jugador
typedef struct{
    char nombre[16]; //nombre del jugador
    unsigned int puntaje; //puntaje del jugador
    unsigned int movs_validos;
    unsigned int movs_invalidos;
    unsigned short x,y; //posicion
    pid_t pid; //identificador del proceso
    bool puede_moverse; // indica si el jugador tiene movimientos validos disponibles
} jugador;

//struct Estado del juego
typedef struct{
    unsigned short width; //ancho del tablero
    unsigned short height; //alto del tablero
    unsigned int cantidad_jugadores; //cantidad total de jugadores
    jugador jugadores[MAX_PLAYERS]; //lista de jugadores
    bool juego_terminado; //indica si el juego termino
    int *tablero;  // Puntero al tablero de juego
} EstadoJuego;

//Estructura para la sincronizacion de procesos
typedef struct{
    sem_t sem_vista; //indica a la vista que hay cambios por imprimir
    sem_t sem_master; //indica al master que la vista termino de imprimir
    sem_t mutex_estado; // Mutex para evitar inanicion del master al acceder al estado
    sem_t mutex_tablero; // Mutex para proteger el acceso al tablero
    sem_t mutex_lectores; // Mutex para la siguiente variable
    unsigned int lectores; // Cantidad de jugadores leyendo el estado
}Sincronizacion;

#endif
