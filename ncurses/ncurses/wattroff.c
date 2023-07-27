
#include <ncurses.h>


int wattroff(WINDOW* win, int attrs) {
    if (attrs & 0xFF)
        win->curColor = 0;

    return OK;
}

