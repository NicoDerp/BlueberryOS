
#include <ncurses.h>
#include <unistd.h>
#include <bits/tty.h>


int wmove(WINDOW* win, int y, int x) {

    win->curx = x;
    win->cury = y;

    int args[] = {y + win->starty, x + win->startx};
    if (ttycmd(TTY_MOVE_CURSOR, args, NULL) == -1)
        return ERR;

    return OK;
}

