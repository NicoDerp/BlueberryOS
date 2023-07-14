
#include <ncurses.h>
#include <stdio.h>


int wclear(WINDOW* win) {

    for (unsigned int y = win->starty; y < win->starty+win->height; y++) {

        ttycmd(TTY_ERASE_SCREEN, NULL, NULL);
        move(y, win->startx);
        for (unsigned int x = 0; x < win->width; x++) {
            putchar(' ');
        }
    }

    return OK;
}

