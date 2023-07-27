
#include <ncurses.h>


int wattron(WINDOW* win, int attrs) {
    if (attrs & 0xFF)
        win->curColor = attrs & 0xFF;

    return OK;
}

