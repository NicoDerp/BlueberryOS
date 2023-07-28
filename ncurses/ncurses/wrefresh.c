
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

            int size = x - startx;
            size = (win->starty + y + 1 == stdscr->height) && (win->startx + x + 1 == stdscr->width) ? size-1 : size;
            if (size == 0) {
                int args[] = {color_pairs[pair].fg + 30, color_pairs[pair].bg + 46};
                ttycmd(TTY_CHANGE_COLOR, args, NULL);
                prevColor = pair;
                continue;
            }

            move(win->starty + y, win->startx + startx);
            memcpy(buf, &win->buf[index + startx], size);
            buf[size] = '\0';
            printf("%s", buf);

            int args[] = {color_pairs[pair].fg + 30, color_pairs[pair].bg + 46};
            ttycmd(TTY_CHANGE_COLOR, args, NULL);
            prevColor = pair;

            startx = x;
        }

        int size = x - startx;
        size = (win->starty + y + 1 == stdscr->height) && (win->startx + x + 1 == stdscr->width) ? size-1 : size;
        if (size > 0) {

            move(win->starty + y, win->startx + startx);
            memcpy(buf, &win->buf[index + startx], size);
            buf[size] = '\0';
            printf("%s", buf);
        }
    }
    free(buf);

    return OK;
}

