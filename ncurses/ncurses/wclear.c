
#include <ncurses.h>
#include <stdio.h>
#include <unistd.h>
#include <bits/tty.h>


int wclear(WINDOW* win) {

    // If the window width goes from start to end of screen
    if (win->width + win->startx == stdscr->width) {

        for (unsigned int y = win->starty; y < win->starty+win->height; y++) {
            move(y, win->startx);
            ttycmd(TTY_ERASE_FROM_CURSOR, NULL, NULL);
        }
    }
    else {

        // Else then manual erasing is required :(
        for (unsigned int y = win->starty; y < win->starty+win->height; y++) {

            //ttycmd(TTY_ERASE_SCREEN, NULL, NULL);
            for (unsigned int x = 0; x < win->width; x++) {
                putchar(' ');
            }
        }
    }
    move(0, 0);

    return OK;
}

