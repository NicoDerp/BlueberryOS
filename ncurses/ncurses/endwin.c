
#include <ncurses.h>
#include <unistd.h>
#include <bits/tty.h>
#include <stdlib.h>


int endwin(void) {

    delwin(stdscr);

    // Restores screen
    ttycmd(TTY_RESET_MODES, NULL, NULL);
    ttycmd(TTY_RESTORE_SCREEN, NULL, NULL);
    //printw("\e[?47l");

    return OK;
}

