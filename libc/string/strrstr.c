
#include <string.h>


char* strrstr(const char* str, const char* sub) {

    unsigned int sublen = strlen(sub);

    if (sublen == 0)
        return str;

    unsigned int i = strlen(str);
    unsigned int j = sublen - 1;
    while ((i != 0) && (j != 0)) {

        if (str[i] == sub[j]) {
            j--;
        } else {
            j = sublen - 1;
        }

        i--;

        if (j == 0)
            return &str[i];
    }

    return NULL;
}

