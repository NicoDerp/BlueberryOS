
#include <ncurses.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>


static inline void print(WINDOW* win, const char* str, size_t len) {

    for (size_t i = 0; i < len; i++) {
        if (!wputcharw(win, str[i]))
            return;
    }
}

static inline void prints(WINDOW* win, const char* str) {

    for (size_t i = 0; str[i]; i++) {
        if (!wputcharw(win, str[i]))
            return;
    }
}

int vmvwprintw(WINDOW* win, int y, int x, const char* format, va_list args) {

    win->cury = y;
    win->curx = x;

    while (*format != 0) {

        if (format[0] == '%' && format[1] != '%') {
            format++;
            if (*format == 'c') {
                char c = (char) va_arg(args, int);
                wputcharw(win, c);
                format++;
            }
            else if (*format == 'd') {
                int i = (int) va_arg(args, int);
                char buf[64];
                itoa(i, buf, 10);

                /*
                putchar(buf[0]+'0');
                putchar(buf[1]+'0');

                printstring("len");
                char buf2[64];
                itoa(len, buf2, 10);
                printstring(buf2);
                printstring(":");
                */

                prints(win, buf);
                format++;
            }
            else if (*format == 'u') {
                unsigned int i = (unsigned int) va_arg(args, unsigned int);
                char buf[64];
                uitoa(i, buf, 10);

                prints(win, buf);
                format++;
            }
            else if (*format == 'x') {
                unsigned int i = (unsigned int) va_arg(args, unsigned int);
                char buf[64];
                uitoa(i, buf, 16);

                prints(win, buf);
                format++;
            }
            else if (*format == 'o') {
                unsigned int i = (unsigned int) va_arg(args, unsigned int);
                char buf[64];
                uitoa(i, buf, 8);

                prints(win, buf);
                format++;
            }
            else if (*format == 's') {
                const char* s = va_arg(args, const char*);

                prints(win, s);
                format++;
            }
        }
        else {
            size_t count = 0;
            while (format[count] != '%' && format[count] != 0) {
                count++;
            }

            print(win, format, count);
            format += count;
        }
    }

    return OK;
}

