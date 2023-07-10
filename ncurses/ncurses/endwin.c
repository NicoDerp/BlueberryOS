
#include <ncurses.h>
#include <stdlib.h>


int endwin(void) {

    free(stdscr);
    printf("\e[2J");
    return OK;
}

