
#include <ncurses.h>
#include <stdio.h>


int erase(void) {
    return werase(stdscr);
}

