
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>


void ls(char* path, bool list, bool showHidden) {

    DIR* pdir = opendir(path);
    if (pdir == NULL) {
        printf("ls: cannot access '%s': No such file or directory\n", path);
        exit(1);
    }

    struct dirent* ent;
    while ((ent = readdir(pdir)) != NULL) {
        printf("name: '%s'\n", ent->d_name);
    }

    if (closedir(pdir) == -1) {
        printf("close failed\n");
        exit(1);
    }

}

void main(int argc, char** argv) {

    printf("openinga\n");
    if (argc == 1) {

        ls("./", false, false);
        exit(0);
    }

    if (argc == 2) {

        ls(argv[1], false, false);
        exit(0);
    }

    if (argc > 3) {
        printf("Usage: ls [OPTION] path\n");
        exit(1);
    }

    bool showHidden = false;
    bool list = false;
    if (strncmp(argv[1], "-", 1) == 0) {

        for (size_t i = 0; argv[1][i] != 0; i++) {

            if (argv[1][i] == 'l') {
                list = true;
            }
            else if(argv[1][i] == 'a') {
                showHidden = true;
            }
        }
    }

    ls(argv[2], list, showHidden);
}

