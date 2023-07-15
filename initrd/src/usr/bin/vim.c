
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>


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

unsigned int cols;
unsigned int rows;


struct {
    unsigned int x;
    unsigned int y;
} cursor;


char* stateToString(state_t st) {
    if      (st == NORMAL)  return "-- NORMAL --";
    else if (st == COMMAND) return "-- COMMAND --";
    else if (st == INSERT)  return "-- INSERT --";
    else                    return "-- UNKNOWN --";
}

void updateTopBar(void) {

    wclear(topBar);
    mvwprintw(topBar, 0, 0, "%s  %s", stateToString(state), currentFile);
    wrefresh(topBar);
}

void parseCommand(char* buf) {

    if (strcmp(buf, "q") == 0) {
        endwin();
        exit(0);
    }
    else {
        wclear(cmdBar);
        mvwprintw(cmdBar, 0, 0, "Not an editor command: %s", buf);
    }

    wrefresh(cmdBar);
}

void displayScreen(char* buf, unsigned int count) {

    size_t i = 0;
    if (count != 0) {
        for (size_t j = 0, i = 0; i < getmaxy(stdscr)-2 && j < count; i++) {
            for (; buf[j] != '\n' && j < count; j++) {
                //putchar(buf[j]);
                printw("%c", buf[j]);
            }
            //putchar('\n');

            if (i != rows-1)
                printw("\n");
            j++;
        }

    }

    // Draw '~' for empty lines
    for (; i < getmaxy(stdscr)-2; i++) {
        printw("~");
        if (i != rows-1)
            printw("\n");
    }

    refresh();
}

void moveCursor(void) {
    move(cursor.x, cursor.y);
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
    noecho();

    /*
    mvaddstr(getmaxy(stdscr)-1, 0, argv[1]);

    attron(A_REVERSE);
    mvaddstr(getmaxy(stdscr)-2, 0, "NORMAL");
    attroff(A_REVERSE);
    */

    cols = getmaxx(stdscr);
    rows = getmaxy(stdscr);

    topBar = newwin(1, cols, rows-2, 0);
    cmdBar = newwin(1, cols, rows-1, 0);

    rows -= 2;
    refresh();
    //box(bar, '2', '2');
    //box(bar, 0, 0);

    struct stat st;
    if (stat(currentFile, &st) == -1) {
        int backup = errno;
        wclear(cmdBar);
        wprintw(cmdBar, "vim: %s: stat error: %s\n", currentFile, strerror(backup));
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
        }

        endwin();
        exit(1);
    }

    displayScreen(buf, count);



    char cmdBuffer[MAX_CMD_BUFFER+1];
    unsigned int cmdIndex = 0;
    int ch;

    state = NORMAL;

    cursor.x = 0;
    cursor.y = 0;

    updateTopBar();
    wclear(cmdBar);
    wrefresh(cmdBar);
    moveCursor();
    while (true) {

        ch = getch();
        if (state == INSERT) {
            if (ch == '\e') {
                state = NORMAL;
                updateTopBar();
                moveCursor();
            }
            else {
                moveCursor();
                printw("%c", ch);
                wrefresh(stdscr);
            }
        }
        else if (state == NORMAL) {

            if (ch == ':') {
                state = COMMAND;
                cmdIndex = 0;
                updateTopBar();
                wclear(cmdBar);
                mvwprintw(cmdBar, 0, 0, ":");
                wrefresh(cmdBar);
            }
            else if (ch == 'i') {
                state = INSERT;
                updateTopBar();
                moveCursor();
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
                moveCursor();
            }
            else if (ch == '\n') {
                state = NORMAL;
                updateTopBar();
                cmdBuffer[cmdIndex] = '\0';
                parseCommand(cmdBuffer);
                moveCursor();
            }
            else if (ch == '\b' || ch == KEY_BACKSPACE) {
                if (cmdIndex == 0)
                    continue;

                mvwprintw(cmdBar, 0, cmdIndex, " ");
                cmdBuffer[cmdIndex] = ' ';
                wmove(cmdBar, 0, cmdIndex--);
                wrefresh(cmdBar);
            }
            else if (ch == 127) {
                wdelch(cmdBar);
                wrefresh(cmdBar);
            }
            else {
                cmdBuffer[cmdIndex++] = ch;
                wprintw(cmdBar, "%c", ch);
                wrefresh(cmdBar);
            }
        }
    }


    endwin();



    if (close(fd) == -1) {
        int backup = errno;
        printf("close error: %s\n", strerror(backup));
        exit(1);
    }
}

