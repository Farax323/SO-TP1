#ifndef SHM_H
#define SHM_H

#include <stddef.h>
#include <sys/types.h>

/*
 * Crea o abre una memoria compartida y la mapea en el espacio de direcciones del proceso.
 *
 * @param name: Nombre del objeto de memoria compartida.
 * @param flags: Banderas de apertura (por ejemplo, O_RDONLY o O_RDWR | O_CREAT).
 * @param mode: Permisos del objeto (por ejemplo, 0666).
 * @param size: Tamaño de la memoria compartida (si se va a crear).
 * @param prot: Flags de protección para el mmap (por ejemplo, PROT_READ o PROT_READ | PROT_WRITE).
 * @param shm_fd: Puntero a entero donde se almacenará el descriptor de archivo.
 *
 * @return Puntero mapeado a la memoria compartida. Si ocurre un error, se muestra el error y se sale del programa.
 */
void *shm_open_custom(const char *name, int flags, mode_t mode, size_t size, int prot, int *shm_fd);

/*
 * Desconecta (munmap) la memoria compartida y cierra el descriptor.
 * @param ptr: Puntero a la memoria mapeada.
 * @param size: Tamaño de la memoria compartida.
 * @param shm_fd: Descriptor de la memoria compartida.
 * @return 0 si se realizó correctamente, -1 en caso de error.
 */
int shm_disconnect(void *ptr, size_t size, int shm_fd);

/*
 * Elimina la memoria compartida (unlink).
 * @param name: El nombre de la memoria compartida.
 * @return 0 si se eliminó correctamente, -1 en caso de error.
 */
int shm_remove(const char *name);

#endif
