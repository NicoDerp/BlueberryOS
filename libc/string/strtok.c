
#include <string.h>


static char* prev;

char* strtok(char* str, const char* delim) {

    if (str != (char*) 0) {
        prev = str;
    }

    for (unsigned int i = 0;; i++) {
        if (prev[i] == 0 && i == 0) {
            return (char*) 0;
        } else if (prev[i] == 0) {
            char* ret = prev;
            prev += i;
            return ret;
        } else if (strchr(delim, prev[i]) != 0) {
            prev[i] = '\0';
            char* ret = prev;
            prev += i + 1;
            return ret;
        }
    }

    return (char*) 0;
}

