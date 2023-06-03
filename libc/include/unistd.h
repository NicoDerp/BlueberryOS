
#ifndef _UNISTD_H
#define _UNISTD_H 1

#include <sys/cdefs.h>
#include <sys/syscall.h>  /* Definition of SYS_* constants */

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#ifdef __cplusplus
extern "C" {
#endif
int syscall(long unsigned int number, ...);

#ifdef __cplusplus
}
#endif
#endif

