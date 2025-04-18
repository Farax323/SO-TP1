#include "../include/player.h"

static int dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
static int dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};

int direccion_hacia(int x0, int y0, int x1, int y1) {
	int dx_ = x1 - x0;
	int dy_ = y1 - y0;
	if (dx_ > 0)
		dx_ = 1;
	else if (dx_ < 0)
		dx_ = -1;
	if (dy_ > 0)
		dy_ = 1;
	else if (dy_ < 0)
		dy_ = -1;
	for (int dir = 0; dir < 8; dir++) {
		if (dx[dir] == dx_ && dy[dir] == dy_)
			return dir;
	}
	return -1;
}

int distancia(int x0, int y0, int x1, int y1) {
	int dx = abs(x1 - x0);
	int dy = abs(y1 - y0);
	return dx > dy ? dx : dy;
}

int jugadores_cerca(int x, int y, EstadoJuego *estado, int mi_id) {
	int count = 0;
	for (unsigned int i = 0; i < estado->cantidad_jugadores; i++) {
		if ((int) i == mi_id)
			continue;
		jugador *j = &estado->jugadores[i];
		if (abs(j->x - x) <= 1 && abs(j->y - y) <= 1)
			count++;
	}
	return count;
}

void calcular_mejor_movimiento(EstadoJuego *estado, jugador *yo, int *mejor_x, int *mejor_y) {
	float mejor_score = -1.0f;
	for (int y = 0; y < estado->height; y++) {
		for (int x = 0; x < estado->width; x++) {
			int val = estado->tablero[y * estado->width + x];
			if (val <= 0)
				continue;
			int dist = distancia(yo->x, yo->y, x, y);
			if (dist == 0 || dist > 1)
				continue;
			int rivales = jugadores_cerca(x, y, estado, yo - estado->jugadores);
			float score = val / (float) (dist + 1 + rivales);
			if (score > mejor_score) {
				mejor_score = score;
				*mejor_x = x;
				*mejor_y = y;
			}
		}
	}
}

void dormir_milisegundos(int milisegundos) {
	struct timespec ts;
	ts.tv_sec = milisegundos / 1000;
	ts.tv_nsec = (milisegundos % 1000) * 1000000;
	nanosleep(&ts, NULL);
}

void procesar_juego(EstadoJuego *estado, Sincronizacion *sync, jugador *yo, int fd_estado, int fd_sync) {
	while (!estado->juego_terminado && !yo->bloqueado) {
		sem_wait(&sync->mutex_tablero);
		int mejor_x = -1, mejor_y = -1;
		calcular_mejor_movimiento(estado, yo, &mejor_x, &mejor_y);
		if (mejor_x != -1 && mejor_y != -1) {
			int dir = direccion_hacia(yo->x, yo->y, mejor_x, mejor_y);
			if (dir != -1) {
				unsigned char direccion = (unsigned char) dir;
				write(STDOUT_FILENO, &direccion, sizeof(direccion));
				sem_post(&sync->mutex_tablero);
				dormir_milisegundos(800);
				continue;
			}
		}
		sem_post(&sync->mutex_tablero);
		dormir_milisegundos(800);
	}
	shm_disconnect(estado, sizeof(EstadoJuego) + estado->width * estado->height * sizeof(int), fd_estado);
	shm_disconnect(sync, sizeof(Sincronizacion), fd_sync);
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		exit(EXIT_FAILURE);
	}

	int width = atoi(argv[1]);
	int height = atoi(argv[2]);
	size_t tam_total = sizeof(EstadoJuego) + width * height * sizeof(int);
	int fd_estado, fd_sync;

	EstadoJuego *estado = shm_open_custom(SHM_STATE, O_RDONLY, 0666, tam_total, PROT_READ, &fd_estado);
	Sincronizacion *sync =
		shm_open_custom(SHM_SYNC, O_RDWR, 0666, sizeof(Sincronizacion), PROT_READ | PROT_WRITE, &fd_sync);

	jugador *yo = NULL;
	for (unsigned int i = 0; i < estado->cantidad_jugadores; i++) {
		if (estado->jugadores[i].pid == getpid()) {
			yo = &estado->jugadores[i];
			break;
		}
	}
	if (!yo) {
		shm_disconnect(estado, tam_total, fd_estado);
		shm_disconnect(sync, sizeof(Sincronizacion), fd_sync);
		exit(EXIT_FAILURE);
	}

	procesar_juego(estado, sync, yo, fd_estado, fd_sync);

	return 0;
}