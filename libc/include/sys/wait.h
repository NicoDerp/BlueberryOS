
#ifndef _WAIT_H
#define _WAIT_H 1

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

pid_t wait(int* status);

#ifdef __cplusplus
}
#endif

#endif

