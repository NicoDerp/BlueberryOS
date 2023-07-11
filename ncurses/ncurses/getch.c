
#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>


int getch(void) {
    char buf;
    read(STDIN_FILENO, &buf, 1);
    return (int) buf;
}

