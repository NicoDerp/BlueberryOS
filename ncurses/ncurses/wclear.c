
#include <ncurses.h>
#include <stdio.h>
#include <unistd.h>
#include <bits/tty.h>
#include <stdlib.h>
#include <string.h>


int wclear(WINDOW* win) {

    win->toclear = 1;

    memset(win->buf, ' ', win->width * win->height);
    memset(win->lineschanged, 0, win->height);

    // If the window width goes from start to end of screen
    if (win->width + win->startx == stdscr->width) {

        if (win->startx == 0) {
            move(win->starty, win->startx);
            for (unsigned int y = 0; y < win->height; y++) {
                ttycmd(TTY_ERASE_FROM_CURSOR, NULL, NULL);
            }
        }
        else {
            for (unsigned int y = win->starty; y < win->starty+win->height; y++) {
                move(y, win->startx);
                ttycmd(TTY_ERASE_FROM_CURSOR, NULL, NULL);
            }
        }
    }
    else {

        char* buf = malloc(win->width + 1);
        memset(buf, ' ', win->width);
        buf[win->width] = '\0';

        // Else then manual erasing is required :(

        if (win->startx == 0) {
            move(win->starty, win->startx);
            for (unsigned int y = win->starty; y < win->starty+win->height; y++) {
                printf("%s\n", buf);
            }

        }
        else {
            for (unsigned int y = win->starty; y < win->starty+win->height; y++) {
                move(y, win->startx);
                printf("%s", buf);
            }
        }

        free(buf);
    }
    move(win->starty, win->startx);

    return OK;
}

