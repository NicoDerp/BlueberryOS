
#include <string.h>


static char* prev;

char* strtok(char* str, const char* delim) {

    if (str != (char*) 0) {
        prev = str;
    }

    for (unsigned int i = 0; prev[i] != 0; i++) {
        if (strchr(delim, prev[i]) != 0) {
            prev[i] = '\0';
            prev = prev + i + 1;
            return prev;
        }
    }
    return (char*) 0;
}

