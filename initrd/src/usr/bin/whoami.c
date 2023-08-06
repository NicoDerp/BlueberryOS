
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>


int main(int argc, char* argv[]) {

    if (argc != 1) {
        printf("Usage: whoami\n");
        exit(1);
    }

    (void) argv;

    struct passwd pwd;
    struct passwd* tmpPwdPtr;
    char pwdBuf[256];
    uid_t uid = getuid();
    int error;

    if ((error = getpwuid_r(uid, &pwd, pwdBuf, sizeof(pwdBuf), &tmpPwdPtr)) != 0) {
        printf("getpwuid error: %d:%s\n", error, strerror(error));
        exit(1);
    }

    printf("%s\n", pwd.pw_name);

    return 0;
}

