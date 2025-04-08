#include "shm.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

void *shm_open_custom(const char *name, int flags, mode_t mode, size_t size, int prot, int *shm_fd) {
	*shm_fd = shm_open(name, flags, mode);
	if (*shm_fd == -1) {
		perror("Error en shm_open");
		exit(EXIT_FAILURE);
	}

	if (flags & O_CREAT) {
		if (flags & O_RDWR) {
			if (fchmod(*shm_fd, mode) == -1) {
				perror("Error al cambiar permisos de /game_sync");
				exit(EXIT_FAILURE);
			}
		}

		if (ftruncate(*shm_fd, size) == -1) {
			perror("Error en ftruncate");
			close(*shm_fd);
			exit(EXIT_FAILURE);
		}
	}

	void *ptr = mmap(NULL, size, prot, MAP_SHARED, *shm_fd, 0);
	if (ptr == MAP_FAILED) {
		perror("Error en mmap");
		close(*shm_fd);
		exit(EXIT_FAILURE);
	}

	return ptr;
}

int shm_disconnect(void *ptr, size_t size, int shm_fd) {
	int ret = 0;
	if (munmap(ptr, size) == -1) {
		perror("Error al desmapear la memoria compartida");
		ret = -1;
	}
	if (close(shm_fd) == -1) {
		perror("Error al cerrar el descriptor de la memoria compartida");
		ret = -1;
	}
	return ret;
}

int shm_remove(const char *name) {
	if (shm_unlink(name) == -1) {
		perror("Error al eliminar la memoria compartida");
		return -1;
	}
	return 0;
}
