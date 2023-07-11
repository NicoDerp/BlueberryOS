
#include <stdio.h>
#include <ncurses.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>


void main(void) {

    initscr();
    raw();

    char* str = "Halla";
    write(STDIN_FILENO, str, strlen(str)+1);

    int ch;
    //printf("\e[6n");
    //printf("\e[6n");

    while ((ch = getchar()) != 'q') {
        printf("Got %d: '%c'\n", ch, ch);
    }
    endwin();
    return;


    keypad(stdscr, TRUE);
    //noecho();

    //box(stdscr, 0, 0);
    wrefresh(stdscr);
    scrollok(stdscr, TRUE);
    while ((ch=wgetch(stdscr)) != 'q') {
        wprintw(stdscr, "%#x - %s\n", ch, keyname(ch));
    }


}

