
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>

#include <bits/memory.h>


#define MAX_CMD_BUFFER 32

typedef enum {
    NORMAL,
    COMMAND,
    INSERT,
} state_t;

typedef struct {
    unsigned int size;
    char* chars;
} row_t;



WINDOW* topBar;
WINDOW* cmdBar;

state_t state;
char* currentFile;

unsigned int cols;
unsigned int rows;

struct {
    row_t* rows;
    unsigned int numrows;
} E;


struct {
    unsigned int x;
    unsigned int y;
} cursor;

void appendRow(char* s, unsigned int linelen) {

    // If rows is NULL then realloc will call malloc for us
    E.rows = realloc(E.rows, sizeof(row_t) * (E.numrows + 1));
    
    row_t* r = &E.rows[E.numrows];
    r->size = linelen;
    r->chars = (char*) malloc(linelen + 1);
    memcpy(r->chars, s, linelen+1);

    E.numrows++;
}

void readFile(char* filename) {

    int fd = open(filename, O_RDONLY | O_CREAT, 0664);
    if (fd == -1) {
        int backup = errno;
        printf("open error: %s\n", strerror(backup));
        exit(1);
    }


    FILE* fp = fdopen(fd, "r");

    char* line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1) {

        while (line[linelen-1] == '\n')
            line[--linelen] = '\0';

        getchar();
        //printf("Appending %d '%s'\n", linelen, line);
        appendRow(line, linelen);
        E.rows = realloc(E.rows, 2*linelen);
    }

    free(line);

    if (fclose(fp) == -1) {
        int backup = errno;
        printf("fclose error: %s\n", strerror(backup));
        exit(1);
    }

    exit(0);
}

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

void displayScreen(void) {

    size_t i = 0;
    for (i = 0; i < rows-2 && i < E.numrows; i++) {

        if (i == rows-1)
            printw("%s", E.rows[i].chars);
        else
            printw("%s\n", E.rows[i].chars);
    }

    // Draw '~' for empty lines
    for (; i < rows-2; i++) {

        if (i == rows-1)
            printw("~");
        else
            printw("~\n");
    }

    refresh();
}

void moveCursor(void) {
    move(cursor.y, cursor.x);
}

void main(int argc, char* argv[]) {


    if (argc != 2)
        return;

    currentFile = argv[1];
    readFile(currentFile);

    for(;;) {}

    initscr();
    noecho();

    cols = getmaxx(stdscr);
    rows = getmaxy(stdscr);

    topBar = newwin(1, cols, rows-2, 0);
    cmdBar = newwin(1, cols, rows-1, 0);

    rows -= 2;
    refresh();

    E.rows = NULL;
    E.numrows = 0;



    char cmdBuffer[MAX_CMD_BUFFER+1];
    unsigned int cmdCursor = 0;
    unsigned int cmdSize = 0;
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
            else if (ch == '\n') {
                cursor.x = 0;
                cursor.y++;
                moveCursor();
            }
            else {
                moveCursor();
                printw("%c", ch);
                wrefresh(stdscr);

                cursor.x++;
                if (cursor.x >= cols) {
                    cursor.x = 0;
                    cursor.y++;
                }
                moveCursor();
            }
        }
        else if (state == NORMAL) {

            if (ch == ':') {
                state = COMMAND;
                cmdCursor = 0;
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

            if (cmdCursor > MAX_CMD_BUFFER) {
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
                cmdBuffer[cmdSize] = '\0';
                parseCommand(cmdBuffer);
                moveCursor();
                cmdSize = 0;
            }
            else if (ch == KEY_LEFT) {
                if (cmdCursor == 0)
                    continue;

                wmove(cmdBar, 0, --cmdCursor+1);
            }
            else if (ch == KEY_RIGHT) {
                if (cmdCursor == cmdSize)
                    continue;

                wmove(cmdBar, 0, ++cmdCursor+1);
            }
            else if (ch == '\b' || ch == KEY_BACKSPACE) {
                if (cmdCursor == 0)
                    continue;

                mvwprintw(cmdBar, 0, cmdCursor, " ");
                cmdBuffer[cmdCursor] = ' ';
                wmove(cmdBar, 0, cmdCursor--);
                wrefresh(cmdBar);
                cmdSize--;
            }
            else if (ch == 127) {
                wdelch(cmdBar);
                wrefresh(cmdBar);
                cmdSize--;
            }
            else {

                if (cmdCursor == cmdSize) {
                    cmdBuffer[cmdCursor++] = ch;
                    cmdSize++;
                    wprintw(cmdBar, "%c", ch);
                }
                else {
                    memmove(&cmdBuffer[cmdCursor+1], &cmdBuffer[cmdCursor], cmdSize-cmdCursor);
                    cmdBuffer[cmdCursor++] = ch;
                    cmdBuffer[++cmdSize] = '\0';
                    wclear(cmdBar);
                    mvwprintw(cmdBar, 0, 0, ":%s", cmdBuffer);
                    wmove(cmdBar, 0, cmdCursor+1);
                }

                wrefresh(cmdBar);
            }
        }
    }


    endwin();
}

