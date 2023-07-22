
#include <stdio.h>
#include <ncurses.h>


int main() {

    initscr();

    printw("Hello world!");
    clear();
    refresh();
    getch();

    endwin();

    return 0;
}

