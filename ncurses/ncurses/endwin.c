
#include <ncurses.h>
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
    printw("\e[?47l");

    return OK;
}

