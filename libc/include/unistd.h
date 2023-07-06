
#ifndef _UNISTD_H
#define _UNISTD_H 1

#include <sys/cdefs.h>
#include <sys/syscall.h>  /* Definition of SYS_* constants */
#include <sys/types.h>

#include <stddef.h>

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

typedef int pid_t;

#ifdef __cplusplus
extern "C" {
#endif
int syscall(long unsigned int number, ...);

int write(int fd, const void* buf, size_t count);
int read(int fd, void* buf, size_t count);

void yield(void);
pid_t fork(void);
int execvp(const char* file, char* const argv[]);
char* getcwd(char* buf, size_t size);
int chdir(const char* path);
int close(int fd);

uid_t getuid(void);
int setuid(uid_t);

#ifdef __cplusplus
}
#endif
#endif

