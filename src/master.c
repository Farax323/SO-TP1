#include "../include/master.h"
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define SHM_STATE "/game_state"
#define SHM_SYNC "/game_sync"

#define MAX_PIPES 9

int direcciones[8][2] = {{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}};

int **crear_pipes(int cantidad);
void cerrar_pipes_excepto(int **pipes, int cantidad, int idx, int extremo);
void colocar_jugadores(EstadoJuego *estado, int *tablero, unsigned int cantidad);
void inicializar_tablero(int *tablero, int width, int height, unsigned int seed);
bool mover_jugador(EstadoJuego *estado, int *tablero, int id, unsigned char dir);
bool jugador_esta_bloqueado(jugador *j, int *tablero, int width, int height);
bool todos_bloqueados(EstadoJuego *estado);

int **crear_pipes(int cantidad) {
    int **pipes = malloc(sizeof(int *) * cantidad);
    for (int i = 0; i < cantidad; i++) {
        pipes[i] = malloc(sizeof(int) * 2);
        pipe(pipes[i]);
    }
    return pipes;
}

void cerrar_pipes_excepto(int **pipes, int cantidad, int idx, int extremo) {
    for (int i = 0; i < cantidad; i++) {
        if (i != idx || extremo == 1)
            close(pipes[i][0]);
        if (i != idx || extremo == 0)
            close(pipes[i][1]);
    }
}

void colocar_jugadores(EstadoJuego *estado, int *tablero, unsigned int cantidad) {
    int ancho = estado->width;
    int alto = estado->height;
    int margen_x = ancho / (cantidad + 1);
    int margen_y = alto / (cantidad + 1);

    for (unsigned int i = 0; i < cantidad; i++) {
        jugador *j = &estado->jugadores[i];
        j->x = margen_x * (i + 1);
        j->y = margen_y * (i + 1);
        if (j->x >= ancho)
            j->x = ancho - 1;
        if (j->y >= alto)
            j->y = alto - 1;
        tablero[j->y * ancho + j->x] = i == 0 ? 0 : -(i);
        j->movs_invalidos = 0;
        j->movs_validos = 0;
        j->puntaje = 0;
        j->bloqueado = false;
        snprintf(j->nombre, sizeof(j->nombre), "Player_%d", i);
    }
}

void inicializar_tablero(int *tablero, int width, int height, unsigned int seed) {
    srand(seed);
    for (int i = 0; i < width * height; i++) {
        tablero[i] = (rand() % 9) + 1;
    }
}

bool mover_jugador(EstadoJuego *estado, int *tablero, int id, unsigned char dir) {
    jugador *j = &estado->jugadores[id];
    int nx = j->x + direcciones[dir][0];
    int ny = j->y + direcciones[dir][1];

    if (nx < 0 || ny < 0 || nx >= estado->width || ny >= estado->height)
        return false;
    int *celda = &tablero[ny * estado->width + nx];
    if (*celda <= 0)
        return false;

    tablero[j->y * estado->width + j->x] = id == 0 ? 0 : -(id); // traza cuerpo
    j->puntaje += *celda;
    *celda = id == 0 ? 0 : -(id);
    j->x = nx;
    j->y = ny;
    j->movs_validos++;
    return true;
}

bool jugador_esta_bloqueado(jugador *j, int *tablero, int width, int height) {
    for (int dir = 0; dir < 8; dir++) {
        int nx = j->x + direcciones[dir][0];
        int ny = j->y + direcciones[dir][1];
        if (nx >= 0 && ny >= 0 && nx < width && ny < height) {
            int val = tablero[ny * width + nx];
            if (val > 0)
                return false;
        }
    }
    return true;
}

bool todos_bloqueados(EstadoJuego *estado) {
    for (unsigned int i = 0; i < estado->cantidad_jugadores; i++) {
        if (!estado->jugadores[i].bloqueado)
            return false;
    }
    return true;
}

int main(int argc, char *argv[]) {
    int width = 10, height = 10, delay = 200, timeout = 10;
    unsigned int seed = time(NULL);
    char *view_path = NULL;
    char *players[MAX_PLAYERS];
    int cant_players = 0;

    int opt;
    while ((opt = getopt(argc, argv, "w:h:d:t:s:v:p:")) != -1) {
        switch (opt) {
        case 'w':
            width = atoi(optarg);
            break;
        case 'h':
            height = atoi(optarg);
            break;
        case 'd':
            delay = atoi(optarg);
            break;
        case 't':
            timeout = atoi(optarg);
            break;
        case 's':
            seed = atoi(optarg);
            break;
        case 'v':
            view_path = optarg;
            break;
        case 'p':
            while (optind <= argc && cant_players < MAX_PLAYERS) {
                players[cant_players++] = argv[optind - 1];
                if (optind < argc && argv[optind][0] != '-')
                    optind++;
                else
                    break;
            }
            break;
        }
    }

    size_t tam_estado = sizeof(EstadoJuego) + width * height * sizeof(int);
    int shm_fd = shm_open(SHM_STATE, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, tam_estado);
    EstadoJuego *estado = mmap(NULL, tam_estado, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    int *tablero = (int *)((char *)estado + sizeof(EstadoJuego));

    int shm_sync = shm_open(SHM_SYNC, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_sync, sizeof(Sincronizacion));
    Sincronizacion *sync = mmap(NULL, sizeof(Sincronizacion), PROT_READ | PROT_WRITE, MAP_SHARED, shm_sync, 0);

    sem_init(&sync->sem_vista, 1, 0);
    sem_init(&sync->sem_master, 1, 0);
    sem_init(&sync->mutex_estado, 1, 1);
    sem_init(&sync->mutex_tablero, 1, 1);
    sem_init(&sync->mutex_lectores, 1, 1);
    sync->lectores = 0;

    estado->width = width;
    estado->height = height;
    estado->cantidad_jugadores = cant_players;
    estado->juego_terminado = false;

    inicializar_tablero(tablero, width, height, seed);
    colocar_jugadores(estado, tablero, cant_players);

    int **pipes = crear_pipes(cant_players);
    pid_t pids[MAX_PLAYERS];

    for (int i = 0; i < cant_players; i++) {
        if ((pids[i] = fork()) == 0) {
            dup2(pipes[i][1], STDOUT_FILENO);
            cerrar_pipes_excepto(pipes, cant_players, i, 1);
            char w_str[5], h_str[5];
            sprintf(w_str, "%d", width);
            sprintf(h_str, "%d", height);
            execl(players[i], players[i], w_str, h_str, NULL);
            perror("exec jugador");
            exit(1);
        } else {
            estado->jugadores[i].pid = pids[i];
        }
    }

    pid_t pid_vista = -1;
    if (view_path) {
        if ((pid_vista = fork()) == 0) {
            char w_str[5], h_str[5];
            sprintf(w_str, "%d", width);
            sprintf(h_str, "%d", height);
            execl(view_path, view_path, w_str, h_str, NULL);
            perror("exec vista");
            exit(1);
        }
    }

    fd_set set;
    int maxfd = 0;
    for (int i = 0; i < cant_players; i++) {
        if (pipes[i][0] > maxfd)
            maxfd = pipes[i][0];
    }

    time_t ultimo_mov = time(NULL);

    while (true) {
        FD_ZERO(&set);
        for (int i = 0; i < cant_players; i++) {
            FD_SET(pipes[i][0], &set);
        }

        struct timeval tv = {1, 0};
        int ready = select(maxfd + 1, &set, NULL, NULL, &tv);

        if (ready > 0) {
            for (int i = 0; i < cant_players; i++) {
                if (FD_ISSET(pipes[i][0], &set)) {
                    unsigned char dir;
                    int n = read(pipes[i][0], &dir, 1);

                    if (n <= 0) {
                        estado->jugadores[i].bloqueado = true;
                        continue;
                    }

                    if (dir < 8 && mover_jugador(estado, tablero, i, dir)) {
                        ultimo_mov = time(NULL);
                    } else {
                        estado->jugadores[i].movs_invalidos++;
                    }

                    if (view_path) {
                        sem_post(&sync->sem_vista);
                        sem_wait(&sync->sem_master);
                    }
                    usleep(delay * 1000);

                    if (!estado->jugadores[i].bloqueado &&
                        jugador_esta_bloqueado(&estado->jugadores[i], tablero, estado->width, estado->height)) {
                        estado->jugadores[i].bloqueado = true;
                        printf("[MASTER] Jugador %d (%s) bloqueado.\n", i, estado->jugadores[i].nombre);
                    }
                    for (unsigned int i = 0; i < estado->cantidad_jugadores; i++) {
                        jugador *j = &estado->jugadores[i];
                        if (!j->bloqueado && jugador_esta_bloqueado(j, tablero, estado->width, estado->height)) {
                            j->bloqueado = true;
                            printf("[MASTER] Jugador %d (%s) bloqueado por evaluación periódica.\n", i, j->nombre);
                        }
                    }
                }
            }
        }

        if (time(NULL) - ultimo_mov >= timeout) {
            printf("[MASTER] Timeout alcanzado. Finalizando juego.\n");
            break;
        }

        if (todos_bloqueados(estado)) {
            printf("[MASTER] Todos los jugadores están bloqueados. Finalizando juego.\n");
            break;
        }
    }

    estado->juego_terminado = true;
    if (view_path)
        sem_post(&sync->sem_vista);

    for (int i = 0; i < cant_players; i++)
        waitpid(pids[i], NULL, 0);
    if (pid_vista != -1)
        waitpid(pid_vista, NULL, 0);

    printf("\n[MASTER] Juego terminado. Puntajes finales:\n");
    for (int i = 0; i < cant_players; i++) {
        jugador *j = &estado->jugadores[i];
        printf("Jugador %d (%s): %u puntos, %u válidos, %u inválidos\n",
            i, j->nombre, j->puntaje, j->movs_validos, j->movs_invalidos);
    }

    return 0;
}