
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <bits/tty.h>


WINDOW* stdscr;

WINDOW* initscr(void) {

    unsigned int width;
    unsigned int height;

    unsigned int* ret[] = {&width, &height};
    if (ttycmd(TTY_GET_MAX_WIN_SIZE, NULL, ret) == -1) {
        printf("ttycmd failed\n");
        free(stdscr);
        exit(1);
    }

    // Restores screen before clearing it
    printf("\e[?47h\e[2J");

    stdscr = newwin(height, width, 0, 0);
    return stdscr;
}

