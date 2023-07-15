
#include <ncurses.h>
#include <stdio.h>


int wrefresh(WINDOW* win) {

    (void) win;

    /*
    for (unsigned int y = 0; y < win->height; y++) {
        move(win->starty + y, win->startx);
        for (unsigned int x = 0; x < win->width; x++) {
            putchar(win->buf[y * win->width + x]);
        }
    }
    */

    return OK;
}

