
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>


#define MAX_CMD_BUFFER 32

typedef enum {
    NORMAL,
    COMMAND,
    INSERT,
} state_t;




WINDOW* topBar;
WINDOW* cmdBar;

state_t state;
char* currentFile;



char* stateToString(state_t st) {
    if      (st == NORMAL)  return "-- NORMAL --";
    else if (st == COMMAND) return "-- COMMAND --";
    else if (st == INSERT)  return "-- INSERT --";
    else                    return "-- UNKNOWN --";
}

void updateTopBar(void) {

    wclear(topBar);
    mvwprintw(topBar, 0, 0, "%s", stateToString(state));
    //mvwprintw(topBar, 0, 0, "%s", stateToString(state), currentFile);
    mvwprintw(topBar, 0, 10, "%s", currentFile);
    wrefresh(topBar);
}

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

    currentFile = argv[1];
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

    topBar = newwin(1, getmaxx(stdscr), getmaxy(stdscr)-2, 0);
    cmdBar = newwin(1, getmaxx(stdscr), getmaxy(stdscr)-1, 0);
    refresh();
    //box(bar, '2', '2');
    //box(bar, 0, 0);

    struct stat st;
    if (stat(currentFile, &st) == -1) {
        int backup = errno;
        printf("vim: %s: stat error: %s\n", currentFile, strerror(backup));
        endwin();
        exit(1);
    }

    char* buf = (char*) malloc(st.st_size);
    int count = read(fd, buf, st.st_size);

    if (count == -1) {
        int backup = errno;
        printf("vim: %s: read error: %s\n", currentFile, strerror(backup));

        if (close(fd) == -1) {
            int backup = errno;
            printf("vim: %s: close error: %s\n", currentFile, strerror(backup));
            endwin();
            exit(1);
        }

        endwin();
        exit(1);
    }

    if (count != 0)
        printw("%s", buf);
    refresh();

    char cmdBuffer[MAX_CMD_BUFFER+1];
    unsigned int cmdIndex = 0;
    int ch;

    state = NORMAL;

    updateTopBar();
    wclear(cmdBar);
    wrefresh(cmdBar);
    while (true) {

        ch = getch();
        if (state == INSERT) {
            if (ch == '\e') {
                state = NORMAL;
                updateTopBar();
            }
            else {
                printw("%c", ch);
            }
        }
        else if (state == NORMAL) {

            if (ch == ':') {
                state = COMMAND;
                cmdIndex = 0;
                updateTopBar();
                wclear(cmdBar);
                wprintw(cmdBar, ":");
            }
            else if (ch == 'i') {
                state = INSERT;
                updateTopBar();
            }
        }
        else if (state == COMMAND) {

            if (cmdIndex > MAX_CMD_BUFFER) {
                wprintw(cmdBar, "Max command buffer size reached\n");
                break;
            }

            if (ch == '\e') {
                state = NORMAL;
                updateTopBar();
                wclear(cmdBar);
            }
            else if (ch == '\n') {
                state = NORMAL;
                updateTopBar();
                cmdBuffer[cmdIndex] = '\0';
                parseCommand(cmdBuffer);
            }
            else {
                cmdBuffer[cmdIndex++] = ch;
                wprintw(cmdBar, "%c", ch);
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

