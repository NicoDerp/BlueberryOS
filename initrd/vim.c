
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>


#define MAX_CMD_BUFFER 32 Hei hallo ooga booga Hei hallo ooga booga Hei hallo ooga booga Hei hallo ooga booga Hei hallo ooga booga Hei hallo ooga booga Hei hallo ooga booga Hei hallo ooga booga Hei hallo ooga booga Hei hallo ooga booga Hei hallo ooga booga Hei hallo ooga booga Hei hallo ooga booga Hei hallo ooga booga Hei hallo ooga booga Hei hallo ooga booga Hei hallo ooga booga Hei hallo ooga booga
#define MAX_CMD_BUFFER 32 Hei hallo ooga booga Hei hallo ooga booga Hei hallo ooga booga

	typedef enum {
a	hei
    NORMAL,
    COMMAND,
    INSERT,
} state_t;

state_t state = NORMAL;

void parseCommand(char* buf) {

    if (strcmp(buf, "q") == 0) {
        endwin();
        exit(0);
    }
    else {

    }

}

void main(int argc, char* argv[]) {

    if (argc != 2)
        return;

    int fd = open(argv[1], O_RDWR | O_CREAT, 0664);
    if (fd == -1) {
        int backup = errno;
        printf("open error: %s\n", strerror(backup));
        exit(1);
    }


    initscr();
    //noecho();

    /*
    mvaddstr(getmaxy(stdscr)-1, 0, argv[1]);

    attron(A_REVERSE);
    mvaddstr(getmaxy(stdscr)-2, 0, "NORMAL");
    attroff(A_REVERSE);
    */

    WINDOW* bar = newwin(2, getmaxx(stdscr), getmaxy(stdscr)-2, 0);
    refresh();
    //box(bar, '2', '2');
    //box(bar, 0, 0);

    mvwprintw(bar, 0, 0, "-- INSERT --", "aioje");
    mvwprintw(bar, 1, 0, "\"%s\"", argv[1]);

    wrefresh(bar);



    char buf[512];
    int count = read(fd, buf, sizeof(buf));
    if (count == -1) {
        int backup = errno;
        printf("read error: %s\n", strerror(backup));
        if (close(fd) == -1) {
            int backup = errno;
            printf("close error: %s\n", strerror(backup));
            exit(1);
        }
        exit(1);
    }

    if (count != 0)
        printw("%s", buf);



    char cmdBuffer[MAX_CMD_BUFFER+1];
    unsigned int cmdIndex;
    int ch;
    while ((ch = getch())) {

        if (ch == ':') {
            state = COMMAND;
            cmdIndex = 0;
        }
        else if (state == COMMAND) {
            if (cmdIndex > MAX_CMD_BUFFER) {
                printf("Max command buffer size reached\n");
                break;
            }

            if (ch == '\n') {
                
                cmdBuffer[cmdIndex] = '\0';
            }
            else {
                cmdBuffer[cmdIndex++] = ch;
            }
        }
    }


    endwin();



    /*
    if (close(fd) == -1) {
        int backup = errno;
        printf("close error: %s\n", strerror(backup));
        exit(1);
    }
    */
}

