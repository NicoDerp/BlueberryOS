
#ifndef _PWD_H
#define _PWD_H 1

#include <sys/cdefs.h>
#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>


struct passwd {
    char* pw_name;
    uid_t pw_uid;
    gid_t pw_gid;
    char* pw_dir;
    char* pw_shell;
};

#ifdef __cplusplus
extern "C" {
#endif

int getpwuid_r(uid_t uid, struct passwd* pwd, char* buffer, size_t bufsize, struct passwd** result);

#ifdef __cplusplus
}
#endif

#endif

