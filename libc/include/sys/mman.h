
#ifndef _SYS_MMAN_H
#define _SYS_MMAN_H 1

#include <sys/cdefs.h>
#include <stddef.h>
#include <stdint.h>


#define PROT_NONE       0x0             /* Page can not be accessed.  */
#define PROT_READ       0x1             /* Page can be read.  */
#define PROT_WRITE      0x2             /* Page can be written.  */
#define PROT_EXEC       0x4             /* Page can be executed.  */

#define MAP_SHARED      0x01            /* Share changes.  */
#define MAP_PRIVATE     0x02            /* Changes are private.  */

#define MAP_FILE        0x10            /* Map file */
#define MAP_FIXED       0x20            /* Interpret addr exactly.  */
#define MAP_ANONYMOUS   0x20            /* Don't use a file.  */
#define MAP_ANON        MAP_ANONYMOUS

#define MAP_FAILED ((void*) -1)

#ifdef __cplusplus
extern "C" {
#endif

void* mmap(void* addr, size_t length, int prot, int flags, int fd, uint32_t offset);
int munmap(void* addr, size_t length);

#ifdef __cplusplus
}
#endif

#endif

