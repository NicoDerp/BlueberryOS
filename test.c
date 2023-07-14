
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>


int main() {

    sleep(2);

    char buf[256];
    read(STDIN_FILENO, &buf, sizeof(buf));
    printf("Read: '%s'\n", buf);

    return 0;

    initscr();
    noecho();
    //raw();
    cbreak();
    keypad(stdscr, TRUE);

    sleep(2);

    int ch;
    while ((ch = getch()) != 'q') {
        printw("Ch: '%d' '%c'\n", ch, ch);
        if (ch == KEY_LEFT)
            printw("Ooga booga\n");
    }


    nocbreak();
    endwin();

    return 0;
}

