
#include <ncurses.h>
#include <stdlib.h>


int delwin(WINDOW* win) {

    free(win->buf);
    free(win->lineschanged);
    free(win->colors);
    free(win);

    return OK;
}

