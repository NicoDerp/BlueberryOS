
#include <ncurses.h>

int refresh(void) {
    return wrefresh(stdscr);
}

