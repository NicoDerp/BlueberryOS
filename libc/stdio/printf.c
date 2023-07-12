
#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int printf(const char* __restrict format, ...) {
    va_list parameters;
    va_start(parameters, format);

    int written = vprintf(format, parameters);

    va_end(parameters);
    return written;
}


