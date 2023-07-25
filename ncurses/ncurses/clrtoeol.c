
#include <ncurses.h>


int clrtoeol(void) {
    return wclrtoeol(stdscr);
}

