
#include <ncurses.h>
#include <stdio.h>
#include <stdarg.h>


int printw(char* format, ...) {

    va_list args;
    va_start(args, format);

    int ret = vmvwprintw(stdscr, stdscr->cury, stdscr->curx, format, args);

    va_end(args);

    return ret;
}

