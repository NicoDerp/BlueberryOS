
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bits/tty.h>
#include <unistd.h>


extern color_pair_t color_pairs[];
unsigned char prevColor = 0;

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
                    printf("%s", buf);
                }

                free(buf);
            }
        }

    }

    char* buf = malloc(win->width + 1);
    for (unsigned int y = 0; y < win->height; y++) {
        if (!win->lineschanged[y])
            continue;

        win->lineschanged[y] = 0;

        unsigned int index = y * win->width;

        unsigned int startx = 0;
        unsigned int x;
        for (x = 0; x < win->width; x++) {

            unsigned char pair = win->colors[index + x];
            if (pair == prevColor)
                continue;

            int args[] = {color_pairs[pair].fg + 30, color_pairs[pair].bg + 46};
            ttycmd(TTY_CHANGE_COLOR, args, NULL);

            if (x > 0) {
                unsigned int size = x - startx;
                memcpy(buf, &win->buf[index + startx], size);
                if (win->starty + y == stdscr->height-1)
                    buf[size - 1] = '\0';
                else
                    buf[size] = '\0';

                move(win->starty + y, win->startx + startx);
                printf("%s", buf);
            }

            prevColor = pair;
            startx = x;
        }

        unsigned int size = x - startx;
        if (size > 0) {
            memcpy(buf, &win->buf[index + startx], size);
            if (win->starty + y == stdscr->height-1)
                buf[size - 1] = '\0';
            else
                buf[size] = '\0';

            move(win->starty + y, win->startx + startx);
            printf("%s", buf);
        }
    }
    free(buf);

    return OK;
}

