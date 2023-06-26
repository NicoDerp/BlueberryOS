
#include <string.h>


char* strchr(const char* str, int c) {

    for (unsigned int i = 0; str[i] != 0; i++) {

        if (str[i] == c) {
            return &str[i];
        }
    }

    return (char*) 0;
}

