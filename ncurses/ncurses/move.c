
#include <ncurses.h>
#include <unistd.h>
#include <bits/tty.h>


int move(int y, int x) {

    int args[] = {y, x};
    if (ttycmd(TTY_MOVE_CURSOR, args, NULL) == -1)
        return ERR;

    return OK;
}

