
#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <bits/types/struct_FILE.h>

#include <unistd.h>
#include <sys/syscall.h>

extern int syscall3(int, int, int, int);


static inline void pchar(int fd, char c) {
    syscall3(SYS_write, fd, (int) c, 1);
}

static inline void print(int fd, const char* data, size_t length) {
    syscall3(SYS_write, fd, (int) data, length);
}

static inline void printstring(int fd, const char* data) {
    size_t length = strlen(data);
    syscall3(SYS_write, fd, (int) data, length);
}

int fprintf(FILE* stream, const char* __restrict format, ...) {

    va_list parameters;
    va_start(parameters, format);

    unsigned int written = 0;
    while (*format != 0) {
        if (format[0] == '%' && format[1] != '%') {
            format++;
            if (*format == 'c') {
                char c = (char) va_arg(parameters, int);
                pchar(stream->dd_fd, c);
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

                print(stream->dd_fd, buf, len);
                format++;
                written += len;
            }
            else if (*format == 'u') {
                unsigned int i = (unsigned int) va_arg(parameters, unsigned int);
                char buf[64];
                uitoa(i, buf, 10);
                size_t len = strlen(buf);

                print(stream->dd_fd, buf, len);
                format++;
                written += len;
            }
            else if (*format == 'x') {
                unsigned int i = (unsigned int) va_arg(parameters, unsigned int);
                char buf[64];
                uitoa(i, buf, 16);
                size_t len = strlen(buf);

                print(stream->dd_fd, buf, len);
                format++;
                written += len;
            }
            else if (*format == 'o') {
                unsigned int i = (unsigned int) va_arg(parameters, unsigned int);
                char buf[64];
                uitoa(i, buf, 8);
                size_t len = strlen(buf);

                print(stream->dd_fd, buf, len);
                format++;
                written += len;
            }
            else if (*format == 's') {
                const char* s = va_arg(parameters, const char*);
                size_t len = strlen(s);
                print(stream->dd_fd, s, len);
                format++;
                written += len;
            }
        }
        else {
            size_t count = 0;
            while (format[count] != '%' && format[count] != 0) {
                count++;
            }
            print(stream->dd_fd, format, count);
            format += count;
            written += count;
        }
    }

    va_end(parameters);

    return written;
}


