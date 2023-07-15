
#include <ncurses.h>
#include <bits/tty.h>
#include <unistd.h>


int wdelch(WINDOW* win) {

    move(win->starty+win->cury, win->startx+win->curx);
    int args[] = {1, win->width - win->cury};
    ttycmd(TTY_DELETE_CHARACTERS, args, NULL);

    /*
    unsigned int index = win->cury * win->height;
    for (unsigned int x = win->curx; x < win->width; x++) {
        win->buf[index + x] = win->buf[index + x + 1];
    }
    */

    return OK;
}

