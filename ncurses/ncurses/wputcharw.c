
#include <ncurses.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>


extern color_pair_t color_pairs[];

bool wputcharw(WINDOW* win, char c) {

    bool place = true;

    if (c == '\n') {
        win->cury++;
        win->curx = 0;
        place = false;
    }

    if (win->curx >= win->width) {

        win->curx = 0;
        win->cury++;
    }

    if (win->cury >= win->height)
        win->cury = win->height-1;
    //    return false;

    if (place) {
        // TODO check if actually move is needed
        //move(win->starty + win->cury, win->startx + win->curx);

        unsigned int index = win->cury * win->width + win->curx;

        if (c == '\t') {
            unsigned int size = 4 - (win->curx % 4);
            memset(&win->buf[index], ' ', size);
            memset(&win->colors[index], win->curColor, size);
            win->curx += size;
        }
        else {
            win->buf[index] = c;
            win->colors[index] = win->curColor;
            win->curx++;
        }

        win->lineschanged[win->cury] = 1;
    }

    return true;
}


