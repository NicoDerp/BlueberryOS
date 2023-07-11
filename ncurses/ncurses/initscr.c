
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <bits/tty.h>


WINDOW* stdscr;

WINDOW* initscr(void) {
    stdscr = (WINDOW*) malloc(sizeof(WINDOW));
    stdscr->curx = 0;
    stdscr->curx = 0;

    unsigned int* ret[] = {&stdscr->maxx, &stdscr->maxy};
    ttycmd(TTY_GET_MAX_WIN_SIZE, NULL, ret);

    // Restores screen before clearing it
    printf("\e[?47h\e[2J");

    return stdscr;
}

