
#include <ncurses.h>


int delch(void) {

    move(stdscr->starty+stdscr->cury, stdscr->startx+stdscr->curx);
    unsigned int index = stdscr->cury * stdscr->height;
    for (unsigned int x = stdscr->curx; x < stdscr->width; x++) {
        stdscr->buf[index + x] = stdscr->buf[index + x + 1];
    }

    return OK;
}

