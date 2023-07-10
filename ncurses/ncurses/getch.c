
#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>


int getch(void) {
    int buf;
    read(STDIN_FILENO, &buf, 1);
    return buf;
}

