
#ifndef _GRP_H
#define _GRP_H 1

#include <sys/cdefs.h>
#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>


struct group {
    char* gr_name;
    gid_t gr_gid;
    char** gr_mem;
};

#ifdef __cplusplus
extern "C" {
#endif

int getgrgid_r(gid_t gid, struct group* grp, char* buffer, size_t bufsize, struct group** result);

#ifdef __cplusplus
}
#endif

#endif

