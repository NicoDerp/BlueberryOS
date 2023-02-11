
#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


void printf(const char* restrict format, ...) {
    va_list parameters;
    va_start(parameters, format);

    unsigned int written = 0;

    while (*format != 0) {
        if (format[0] == '%' && format[1] != '%') {
            format++;
            if (*format == 'c') {
                char c = (char) va_arg(parameters, int);
                terminal_writechar(c);
                written++;
            }
            else if (*format == 's') {
                const char* s = va_arg(parameters, const char*);
                size_t len = strlen(s);
                terminal_write(s, len);
                format += len;
                written += len;
            }
        }
        else {
            size_t count = 0;
            while (format[count] != '%' && format[count] != 0) {
                count++;
            }
            terminal_write(format, count);
            format += count;
            written += count;
        }
    }

    va_end(parameters);
    return written;
}


