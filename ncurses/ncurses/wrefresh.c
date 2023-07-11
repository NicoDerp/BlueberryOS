
#include <ncurses.h>
#include <stdio.h>


int wrefresh(WINDOW* win) {

    (void) win;
    //move(win->starty, win->startx);
    //printf("%s", win->buf);
    return OK;
}

