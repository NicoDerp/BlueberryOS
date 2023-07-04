
#ifndef _DIRENT_H
#define _DIRENT_H 1

#include <sys/cdefs.h>
#include <stdint.h>
#include <stddef.h>

struct dirent {
    uint32_t d_ino;
    uint32_t d_off;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[256];
};

enum {
    DT_UNKNOWN = 0,
#define DT_UNKNOWN 0

    DT_DIR = 1,
#define DT_DIR 1

    DT_REG = 2,
#define DT_REG 2
};

struct __dirstream;
typedef struct __dirstream DIR;


#ifdef __cplusplus
extern "C" {
#endif


__attribute__((__nonnull__))
DIR* opendir (const char*);

__attribute__((__nonnull__))
struct dirent* readdir(DIR*);

__attribute__((__nonnull__))
int closedir (DIR*);

int getdirentries(int, char*, size_t, uint32_t* __restrict);

#ifdef __cplusplus
}
#endif

#endif

