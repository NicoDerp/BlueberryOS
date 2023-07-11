
#include <ncurses.h>
#include <stdio.h>
#include <stdarg.h>


int printw(char* format, ...) {

    // Goofy asf
    return wprintw(stdscr, format, *((unsigned int*) ((int) &format + sizeof(unsigned int))));
}

