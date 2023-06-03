
#ifndef _SYS_UNISTD_H
#define _SYS_UNISTD_H 1

#include <sys/cdefs.h>
#include <sys/syscall.h>  /* Definition of SYS_* constants */


#ifdef __cplusplus
extern "C" {
#endif
int syscall(long unsigned int number, ...);

#ifdef __cplusplus
}
#endif
#endif

