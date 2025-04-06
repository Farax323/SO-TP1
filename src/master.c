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

int direcciones[8][2] = {{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}};

void *crear_shm(char *name, size_t size, int *shm_fd) {
    *shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (*shm_fd == -1) {
        perror("Error al crear memoria compartida");
        exit(EXIT_FAILURE);
    }

    if (strcmp(name, SHM_SYNC) == 0) {
        if (fchmod(*shm_fd, 0666) == -1) {
            perror("Error al cambiar permisos de /game_sync");
            exit(EXIT_FAILURE);
        }
    }

    if (ftruncate(*shm_fd, size) == -1) {
        perror("Error al truncar memoria compartida");
        exit(EXIT_FAILURE);
    }

    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, *shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("Error al mapear memoria compartida");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

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

void colocar_jugadores(EstadoJuego *estado, unsigned int cantidad) {
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
        estado->tablero[j->y * ancho + j->x] = i == 0 ? 0 : -(i);
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

bool mover_jugador(EstadoJuego *estado, int id, unsigned char dir) {

    int *tablero = estado->tablero;

    jugador *j = &estado->jugadores[id];
    int nx = j->x + direcciones[dir][0];
    int ny = j->y + direcciones[dir][1];

    if (nx < 0 || ny < 0 || nx >= estado->width || ny >= estado->height)
        return false;
    int *celda = &tablero[ny * estado->width + nx];
    if (*celda <= 0)
        return false;

    tablero[j->y * estado->width + j->x] = id == 0 ? 0 : -(id);
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

void dormir_milisegundos(int milisegundos) {
    struct timespec ts;
    ts.tv_sec = milisegundos / 1000;
    ts.tv_nsec = (milisegundos % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

bool todos_bloqueados(EstadoJuego *estado) {
    for (unsigned int i = 0; i < estado->cantidad_jugadores; i++) {
        if (!estado->jugadores[i].bloqueado)
            return false;
    }
    return true;
}

void procesar_argumentos(int argc, char *argv[], int *width, int *height, int *delay, int *timeout, unsigned int *seed, char **view_path, char *players[], int *cant_players) {
    int opt;
    while ((opt = getopt(argc, argv, "w:h:d:t:s:v:p:")) != -1) {
        switch (opt) {
        case 'w':
            *width = atoi(optarg);
            break;
        case 'h':
            *height = atoi(optarg);
            break;
        case 'd':
            *delay = atoi(optarg);
            break;
        case 't':
            *timeout = atoi(optarg);
            break;
        case 's':
            *seed = atoi(optarg);
            break;
        case 'v':
            *view_path = optarg;
            break;
        case 'p':
            while (optind <= argc && *cant_players < MAX_PLAYERS) {
                players[(*cant_players)++] = argv[optind - 1];
                if (optind < argc && argv[optind][0] != '-')
                    optind++;
                else
                    break;
            }
            break;
        }
    }
}

void imprimir_puntajes_finales(EstadoJuego *estado, int cant_players) {
    printf("\n[MASTER] Juego terminado. Puntajes finales:\n");
    for (int i = 0; i < cant_players; i++) {
        jugador *j = &estado->jugadores[i];
        printf("Jugador %d (%s): %u puntos, %u válidos, %u inválidos\n",
            i, j->nombre, j->puntaje, j->movs_validos, j->movs_invalidos);
    }
}

void inicializar_procesos(EstadoJuego *estado, int **pipes, pid_t *pids, int cant_players, char *players[], char *view_path, int width, int height, pid_t *pid_vista) {
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

    *pid_vista = -1;
    if (view_path) {
        if ((*pid_vista = fork()) == 0) {
            char w_str[5], h_str[5];
            sprintf(w_str, "%d", width);
            sprintf(h_str, "%d", height);
            execl(view_path, view_path, w_str, h_str, NULL);
            perror("exec vista");
            exit(1);
        }
    }
}

void inicializar_juego(EstadoJuego **estado, Sincronizacion **sync,
    int width, int height, unsigned int seed, int cant_players, int *shm_fd_estado, int *shm_fd_sync) {

    size_t tam_estado = sizeof(EstadoJuego) + width * height * sizeof(int);
    *estado = crear_shm(SHM_STATE, tam_estado, shm_fd_estado);
    *sync = crear_shm(SHM_SYNC, sizeof(Sincronizacion), shm_fd_sync);

    sem_init(&(*sync)->sem_vista, 1, 0);
    sem_init(&(*sync)->sem_master, 1, 0);
    sem_init(&(*sync)->mutex_estado, 1, 1);
    sem_init(&(*sync)->mutex_tablero, 1, 1);
    sem_init(&(*sync)->mutex_lectores, 1, 1);
    (*sync)->lectores = 0;

    (*estado)->width = width;
    (*estado)->height = height;
    (*estado)->cantidad_jugadores = cant_players;
    (*estado)->juego_terminado = false;

    inicializar_tablero((*estado)->tablero, width, height, seed);
    colocar_jugadores(*estado, cant_players);
}

void procesar_entrada_jugador(EstadoJuego *estado, int jugador_id, int pipe_fd, Sincronizacion *sync, char *view_path, int delay, time_t *ultimo_mov) {
    unsigned char dir;
    int n = read(pipe_fd, &dir, 1);

    if (n <= 0) {
        estado->jugadores[jugador_id].bloqueado = true;
        return;
    }
    sem_wait(&sync->mutex_tablero);
    if (dir < 8 && mover_jugador(estado, jugador_id, dir)) {
        *ultimo_mov = time(NULL);
    } else {
        estado->jugadores[jugador_id].movs_invalidos++;
    }
    sem_post(&sync->mutex_tablero);

    if (view_path) {
        sem_post(&sync->sem_vista);
        sem_wait(&sync->sem_master);
    }

    dormir_milisegundos(delay);
}

void evaluar_bloqueos(EstadoJuego *estado, Sincronizacion *sync) {
    sem_wait(&sync->mutex_lectores);
    if (sync->lectores == 0) {
        sem_wait(&sync->mutex_estado);
    }
    sync->lectores++;
    sem_post(&sync->mutex_lectores);

    for (unsigned int i = 0; i < estado->cantidad_jugadores; i++) {
        jugador *j = &estado->jugadores[i];
        if (!j->bloqueado && jugador_esta_bloqueado(j, estado->tablero, estado->width, estado->height)) {
            j->bloqueado = true;
        }
    }

    sem_wait(&sync->mutex_lectores);
    sync->lectores--;
    if (sync->lectores == 0) {
        sem_post(&sync->mutex_estado);
    }
    sem_post(&sync->mutex_lectores);
}

bool verificar_condiciones_finalizacion(EstadoJuego *estado, time_t ultimo_mov, int timeout) {
    if (time(NULL) - ultimo_mov >= timeout) {
        printf("[MASTER] Timeout alcanzado. Finalizando juego.\n");
        return true;
    }

    if (todos_bloqueados(estado)) {
        printf("[MASTER] Todos los jugadores están bloqueados. Finalizando juego.\n");
        return true;
    }

    return false;
}

int main(int argc, char *argv[]) {
    int width = 10, height = 10, delay = 200, timeout = 10;
    unsigned int seed = time(NULL);
    char *view_path = NULL;
    char *players[MAX_PLAYERS];
    int cant_players = 0;
    int shm_fd_estado, shm_fd_sync;

    procesar_argumentos(argc, argv, &width, &height, &delay, &timeout, &seed, &view_path, players, &cant_players);

    EstadoJuego *estado;

    Sincronizacion *sync;

    inicializar_juego(&estado, &sync, width, height, seed, cant_players, &shm_fd_estado, &shm_fd_sync);

    int **pipes = crear_pipes(cant_players);
    pid_t pids[MAX_PLAYERS];
    pid_t pid_vista;

    inicializar_procesos(estado, pipes, pids, cant_players, players, view_path, width, height, &pid_vista);

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
                    procesar_entrada_jugador(estado, i, pipes[i][0], sync, view_path, delay, &ultimo_mov);
                }
            }
        }

        evaluar_bloqueos(estado, sync);

        if (verificar_condiciones_finalizacion(estado, ultimo_mov, timeout)) {
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

    for (int i = 0; i < cant_players; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
        free(pipes[i]);
    }
    free(pipes);

    imprimir_puntajes_finales(estado, cant_players);

    munmap(estado, sizeof(EstadoJuego) + width * height * sizeof(int));
    close(shm_fd_estado);
    shm_unlink(SHM_STATE);

    munmap(sync, sizeof(Sincronizacion));
    close(shm_fd_sync);
    shm_unlink(SHM_SYNC);

    return 0;
}