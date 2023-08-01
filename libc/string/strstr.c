
#include <string.h>


char* strstr(const char* str, const char* sub) {

    int set = 0;
    unsigned int start = 0;
    unsigned int sublen = strlen(sub);
    unsigned int i = 0;
    unsigned int j = 0;
    while ((str[i] != '\0') && (sub[j] != '\0')) {

        if (j == sublen-1)
            return &str[start];

        if (str[i] == sub[j]) {
            if (!set) {
                set = 1;
                start = i;
            }
            j++;
        } else {
            j = 0;
            set = 0;
        }
        i++;
    }

    return NULL;
}

