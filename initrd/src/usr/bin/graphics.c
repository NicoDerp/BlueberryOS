
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>


void main(void) {

    initscr();
    printw("Hello world!\n");
    refresh();
    getch();
    endwin();
}

