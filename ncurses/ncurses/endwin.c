
#include <ncurses.h>
#include <unistd.h>
#include <bits/tty.h>
#include <stdlib.h>


extern WINDOW* firstwin;

int endwin(void) {

    WINDOW* win = firstwin;
    while (win) {

        free(win->buf);
        free(win);

        win = win->next;
    }

    // Restores screen
    ttycmd(TTY_RESTORE_SCREEN, NULL, NULL);
    //printw("\e[?47l");

    return OK;
}

