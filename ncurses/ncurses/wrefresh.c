
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bits/tty.h>
#include <unistd.h>


int wrefresh(WINDOW* win) {

    if (win->toclear) {
        win->toclear = 0;

        if (win == stdscr) {
            ttycmd(TTY_ERASE_SCREEN, NULL, NULL);
        }
        else {

            // If it ends at end of screen
            if (win->startx + win->width == stdscr->width) {
                move(win->starty, win->startx);
                for (unsigned int i = 0; i < win->height; i++) {
                    ttycmd(TTY_ERASE_FROM_CURSOR, NULL, NULL);
                }
            }
            else {

                char* buf = malloc(win->width+1);
                memset(buf, ' ', win->width);
                buf[win->width] = '\0';

                for (unsigned int i = 0; i < win->height; i++) {
                    move(win->starty+i, win->startx);
                    printf(buf);
                }

                free(buf);
            }
        }

    }

    char* buf = malloc(win->width+1);
    for (unsigned int y = 0; y < win->height; y++) {
        if (!win->lineschanged[y])
            continue;

        win->lineschanged[y] = 0;

        unsigned int index = y * win->width;

        memcpy(buf, &win->buf[index], win->width);
        if (win->starty + y == stdscr->height-1)
            buf[win->width - 1] = '\0';
        else
            buf[win->width] = '\0';

        move(win->starty + y, win->startx);
        printf(buf);
    }
    free(buf);
    printf("Buf at 0x%x - 0x%x\n", buf, buf + win->width+1);

    return OK;
}

