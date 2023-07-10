
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>


WINDOW* stdscr;

WINDOW* initscr(void) {
    stdscr = (WINDOW*) malloc(sizeof(WINDOW));
    stdscr->curx = 0;
    stdscr->curx = 0;

    printf("\e[2J");

    return stdscr;
}

