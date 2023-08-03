
#include <string.h>


char* strrchr(const char* str, int c) {

    for (int i = strlen(str); i >= 0; i--) {

        if (str[i] == c) {
            return &str[i];
        }
    }

    return (char*) 0;
}

