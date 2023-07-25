
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bits/tty.h>
#include <unistd.h>


int wclrtoeol(WINDOW* win) {

    unsigned int index = win->cury * win->width + win->curx;
    memset(win->buf + index, 'o', win->width - win->curx);
    win->lineschanged[win->cury] = 1;
    move(win->cury, win->curx);

    return OK;
}

