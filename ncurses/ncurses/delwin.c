
#include <ncurses.h>
#include <stdlib.h>


int delwin(WINDOW* win) {

    if (win->prev)
        win->prev->next = win->next;

    if (win->next)
        win->next->prev = win->prev;

    free(win->buf);
    free(win);

    return OK;
}

