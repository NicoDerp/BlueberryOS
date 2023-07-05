
#ifndef _SHADOW_H
#define _SHADOW_H 1

#include <sys/cdefs.h>
#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>


struct spwd {
    char *sp_namp;     /* Login name */
    char *sp_pwdp;     /* Encrypted password */
    long  sp_lstchg;   /* Date of last change
                          (measured in days since
                          1970-01-01 00:00:00 +0000 (UTC)) */
    long  sp_min;      /* Min # of days between changes */
    long  sp_max;      /* Max # of days between changes */
    long  sp_warn;     /* # of days before password expires
                          to warn user to change it */
    long  sp_inact;    /* # of days after password expires
                          until account is disabled */
    long  sp_expire;   /* Date when account expires
                          (measured in days since
                          1970-01-01 00:00:00 +0000 (UTC)) */
    unsigned long sp_flag;  /* Reserved */
};

#ifdef __cplusplus
extern "C" {
#endif

int getspnam_r(const char*, struct spwd*, char*, size_t, struct spwd**);

#ifdef __cplusplus
}
#endif

#endif

