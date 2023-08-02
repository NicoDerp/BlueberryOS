
#include <ctype.h>

int isspace(int arg) {
    return (arg == ' ') || ('\t' <= arg && arg <= '\r');
}

