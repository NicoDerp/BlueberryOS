
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bits/tty.h>
#include <unistd.h>


int wclrtoeol(WINDOW* win) {

    memset(&win->buf[win->cury * win->width + win->curx], ' ', win->width - win->curx);
    win->lineschanged[win->cury] = 1;

    return OK;
}

