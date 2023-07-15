
#include <ncurses.h>
//#include <stdio.h>


int clear(void) {
    //printf("\e[2J");
    //return OK;
    
    return wclear(stdscr);
}

