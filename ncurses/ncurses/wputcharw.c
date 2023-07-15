
#include <ncurses.h>
#include <stdio.h>
#include <stdbool.h>


bool wputcharw(WINDOW* win, char c) {

    bool place = true;

    if (c == '\n') {
        win->cury++;
        win->curx = 0;
        place = false;
    }

    if (win->curx >= win->width) {

        win->curx = 0;
        win->cury++;
    }

    if (win->cury >= win->height)
        return false;

    // TODO check if actually move is needed
    move(win->starty + win->cury, win->startx + win->curx);

    if (place) {
        //unsigned int index = win->cury * win->width + win->curx;

        putchar(c);
        //win->buf[index] = c;
        win->curx++;
    }


    return true;
}


