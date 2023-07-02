
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>


void ls(char* path, bool showHidden, bool list) {

    DIR* pdir = opendir(path);
    if (pdir == NULL) {
        printf("ls: cannot access '%s': No such directory\n", path);
        exit(1);
    }

    struct dirent* ent;
    while ((ent = readdir(pdir)) != NULL) {

        if (strncmp(ent->d_name, ".", 1) == 0 && !showHidden)
            continue;

        if (ent->d_type == DT_DIR)
            printf("\e[9;0m%s\e[0m", ent->d_name);
        else
            printf("%s", ent->d_name);

        if (list)
            putchar('\n');
        else
            printf("  ");
    }

    if (!list)
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

