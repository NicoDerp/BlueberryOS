
#include <ncurses.h>
#include <stdio.h>


int clear(void) {

    printf("\e[2J");
    move(0, 0);
    return OK;

    //return wclear(stdscr);
}

