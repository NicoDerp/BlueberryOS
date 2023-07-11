
#include <ncurses.h>
#include <stdlib.h>


int endwin(void) {

    free(stdscr);

    // Restores screen
    printw("\e[?47l");

    return OK;
}

