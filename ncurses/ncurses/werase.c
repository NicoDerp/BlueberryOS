
#include <ncurses.h>
#include <stdio.h>
#include <unistd.h>
#include <bits/tty.h>
#include <stdlib.h>
#include <string.h>


int werase(WINDOW* win) {

    memset(win->buf, ' ', win->width * win->height);
    memset(win->lineschanged, 1, win->height);

    return OK;
}

