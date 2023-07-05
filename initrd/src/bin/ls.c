
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>


void ls(char* path, bool showHidden, bool list) {

    DIR* pdir = opendir(path);
    if (pdir == NULL) {
        int backup = errno;
        printf("ls: cannot access '%s': %s\n", path, strerror(backup));
        exit(1);
    }

    struct passwd passwdStruct;
    struct passwd* tempPwdPointer;
    char passwdBuf[256];

    struct group groupStruct;
    struct group* tempGrpPointer;
    char groupBuf[256];

    struct stat statStruct;
    struct dirent* ent;
    bool empty = true;
    while ((ent = readdir(pdir)) != NULL) {

        if (strncmp(ent->d_name, ".", 1) == 0 && !showHidden)
            continue;

        if (list) {
            if (lstat(ent->d_name, &statStruct) == -1) {
                printf("lstat failed\n");
                exit(1);
            }

            if (getpwuid_r(statStruct.st_uid, &passwdStruct, passwdBuf, sizeof(passwdBuf), &tempPwdPointer) != 0) {
                printf("getpwuid error\n");
                continue;
            }

            if (getgrgid_r(statStruct.st_gid, &groupStruct, groupBuf, sizeof(groupBuf), &tempGrpPointer) != 0) {
                printf("getgrgid error\n");
                continue;
            }

            char sizeBuf[64];
            itoa(statStruct.st_size, sizeBuf, 10);
            int sizeLen = strlen(sizeBuf);

            char mode[] = "----------";
            if (S_ISDIR(statStruct.st_mode))
                mode[0] = 'd';

            for (int i = 0; i < 3; i++) {
                if ((statStruct.st_mode >> 3*i) & 1)
                    mode[6 - 3*i + 3] = 'x';

                if ((statStruct.st_mode >> 3*i) & 2)
                    mode[6 - 3*i + 2] = 'w';

                if ((statStruct.st_mode >> 3*i) & 4)
                    mode[6 - 3*i + 1] = 'r';
            }
            printf("%s", mode);

            for (int i = 0; i < (6-sizeLen); i++) { putchar(' '); }

            printf("%s %s %s ", sizeBuf, passwdStruct.pw_name, groupStruct.gr_name);
        }

        if (ent->d_type == DT_DIR)
            printf("\e[9;0m%s\e[0m", ent->d_name);
        else
            printf("%s", ent->d_name);

        if (list)
            putchar('\n');
        else
            printf("  ");

        empty = false;
    }

    if (!list && !empty)
        putchar('\n');

    if (closedir(pdir) == -1) {
        printf("close failed\n");
        exit(1);
    }

}

bool parseArg(char* arg, bool* showHidden, bool* list) {

    *list = false;
    *showHidden = false;

    if (strncmp(arg, "-", 1) == 0) {

        for (size_t i = 0; arg[i] != 0; i++) {

            if (arg[i] == 'l') {
                *list = true;
            }
            else if(arg[i] == 'a') {
                *showHidden = true;
            }
        }

        return true;
    }

    return false;
}

void main(int argc, char** argv) {

    bool showHidden;
    bool list;

    if (argc == 1) {

        ls("./", false, false);
        exit(0);
    }

    if (argc == 2) {

        if (parseArg(argv[1], &showHidden, &list))
            ls("./", showHidden, list);
        else
            ls(argv[1], false, false);

        exit(0);
    }

    if (argc > 3) {
        printf("Usage: ls [OPTION] path\n");
        exit(1);
    }

    if (!parseArg(argv[1], &showHidden, &list)) {
        printf("Usage: ls [OPTION] path\n");
        exit(1);
    }

    ls(argv[2], showHidden, list);
}

