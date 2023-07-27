
#include <ncurses.h>
#include <unistd.h>
#include <bits/tty.h>


int move(int y, int x) {

    stdscr->curx = x;
    stdscr->cury = y;

    if (ttycmd(TTY_MOVE_CURSOR, (int[]) {y, x}, NULL) == -1)
        return ERR;

    return OK;
}

