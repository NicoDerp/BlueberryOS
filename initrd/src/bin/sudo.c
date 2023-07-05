
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <shadow.h>

#include <sys/types.h>
#include <pwd.h>
#include <grp.h>


void getpass(char* buf, size_t buflen) {
    char c;
    size_t i;
    for (i = 0; i < buflen && (c = getchar()) != '\n'; i++) {
        buf[i] = c;
    }
    buf[i] = '\0';
    putchar('\n');
}

bool authenticateUser(void) {

    struct passwd pwd;
    char pwdBuf[256];
    struct passwd* tmpPwdPtr;
    int error;

    if ((error = getpwuid_r(getuid(), &pwd, pwdBuf, sizeof(pwdBuf), &tmpPwdPtr)) != 0) {
        printf("getpwuid error: %d:%s\n", error, strerror(error));
        exit(1);
    }

    struct spwd spw;
    char spwBuf[256];
    struct spwd* tmpSpwPtr;

    if ((error = getspnam_r(pwd.pw_name, &spw, spwBuf, sizeof(spwBuf), &tmpSpwPtr)) != 0) {
        printf("getspnam error: %d:%s\n", error, strerror(error));
        exit(1);
    }

    printf("[sudo]: password for %s: ", pwd.pw_name);

    char passBuf[256];
    getpass(passBuf, sizeof(passBuf));
    return strcmp(passBuf, spw.sp_pwdp) == 0;
}

void main(int argc, char* argv[]) {

    if (argc == 1) {
        printf("usage: sudo <command>\n");
        exit(0);
    }

    if (!authenticateUser()) {
        printf("Invalid password!\n");
        exit(1);
    }

    if (execvp(argv[1], &argv[1]) == -1) {
        printf("sudo: %s: command not found\n", argv[1]);
        exit(1);
    }

}


