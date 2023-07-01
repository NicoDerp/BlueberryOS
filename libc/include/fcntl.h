
#ifndef _FCNTL_H
#define _FCNTL_H 1


#define O_RDONLY 1
#define O_WRONLY 2
#define O_RDWR (O_RDONLY | O_WRONLY)

#ifdef __cplusplus
extern "C" {
#endif

int open(const char*, int);

#ifdef __cplusplus
}
#endif
#endif

