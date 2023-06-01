
#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(__is_libk)

#include <kernel/tty.h>

static inline void print(const char* data, size_t length) {
    terminal_write(data, length);
}

static inline void printstring(const char* data) {
    terminal_writestring(data);
}

#else

// TODO implement stdio and write system call

#error "System calls aren't implemented yet"

#endif

int printf(const char* restrict format, ...) {
    va_list parameters;
    unsigned int written = 0;

    va_start(parameters, format);
    while (*format != 0) {
        if (format[0] == '%' && format[1] != '%') {
            format++;
            if (*format == 'c') {
                char c = (char) va_arg(parameters, int);
                putchar(c);
                format++;
                written++;
            }
            else if (*format == 'd') {
                int i = (int) va_arg(parameters, int);
                char buf[64];
                itoa(i, buf, 10);
                size_t len = strlen(buf);

                /*
                putchar(buf[0]+'0');
                putchar(buf[1]+'0');

                printstring("len");
                char buf2[64];
                itoa(len, buf2, 10);
                printstring(buf2);
                printstring(":");
                */

                print(buf, len);
                format++;
                written += len;
            }
            else if (*format == 'u') {
                unsigned int i = (unsigned int) va_arg(parameters, unsigned int);
                char buf[64];
                uitoa(i, buf, 10);
                size_t len = strlen(buf);

                print(buf, len);
                format++;
                written += len;
            }
            else if (*format == 'x') {
                unsigned int i = (unsigned int) va_arg(parameters, unsigned int);
                char buf[64];
                uitoa(i, buf, 16);
                size_t len = strlen(buf);

                print(buf, len);
                format++;
                written += len;
            }
            else if (*format == 's') {
                const char* s = va_arg(parameters, const char*);
                size_t len = strlen(s);
                print(s, len);
                format++;
                written += len;
            }
        }
        else {
            size_t count = 0;
            while (format[count] != '%' && format[count] != 0) {
                count++;
            }
            print(format, count);
            format += count;
            written += count;
        }
    }

    va_end(parameters);
    return written;
}


