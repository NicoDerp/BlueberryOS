
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

WINDOW* topBar;
WINDOW* cmdBar;

state_t state;
char* currentFile;

unsigned int maxcols;
unsigned int maxrows;


typedef void (*handler_t)(void);
typedef struct {
    int key;
    handler_t handler;
} mapping_t;

typedef struct {
    unsigned int len;
    char* chars;
} row_t;

struct {
    row_t* rows;
    unsigned int numrows;
    unsigned int rowoff;
    unsigned int coloff;
    unsigned int scurx;
    unsigned int curx;
    unsigned int cury;
} E;


void appendRow(char* s, unsigned int linelen) {

    // If maxrows is NULL then realloc will call malloc for us
    E.rows = realloc(E.rows, sizeof(row_t) * (E.numrows + 1));

    E.rows[E.numrows].len = linelen;
    E.rows[E.numrows].chars = (char*) malloc(linelen + 1);
    memcpy(E.rows[E.numrows].chars, s, linelen+1);

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

        //printf("Appending '%s' with len %d\n", line, linelen);
        appendRow(line, linelen);
    }

    //printf("Row size %s\n", E.rows[0].chars);

    free(line);

    if (fclose(fp) == -1) {
        int backup = errno;
        printf("fclose error: %s\n", strerror(backup));
        exit(1);
    }
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

void clearCmdBar(void) {

    wclear(cmdBar);
    wrefresh(cmdBar);
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

    clear();

    char* buf = NULL;
    size_t i = 0;
    for (i = 0; (i < maxrows) && (i < E.numrows-E.rowoff); i++) {

        unsigned int len = strlen(E.rows[i + E.rowoff].chars);
        if (len < E.coloff) {
            printw("\n");
            continue;
        }

        len -= E.coloff;
        len = len > maxcols ? maxcols : len;

        buf = realloc(buf, len+1);
        memcpy(buf, E.rows[i + E.rowoff].chars + E.coloff, len);
        buf[len] = '\0';

        if (i == maxrows-1)
            printw("%s", buf);
        else
            printw("%s\n", buf);

        free(buf);
    }

    // Draw '~' for empty lines
    for (; i < maxrows; i++) {

        if (i == maxrows-1)
            printw("~");
        else
            printw("~\n");
    }

    updateTopBar();
    clearCmdBar();
    refresh();
}

void scrollUp(void) {

    if (E.rowoff == 0)
        return;

    E.rowoff--;
    displayScreen();

    updateTopBar();
    clearCmdBar();
}

void scrollDown(void) {

    if (E.rowoff + maxrows == E.numrows)
        return;

    E.rowoff++;
    displayScreen();
}

void scrollLeft(void) {

    E.coloff--;
    displayScreen();
}

void scrollRight(void) {

    E.coloff++;
    displayScreen();
}

void moveCursor(void) {
    move(E.cury - E.rowoff, E.curx - E.coloff);
}



void leftArrow(void) {

    if ((E.curx - E.coloff == 0) && (E.cury - E.rowoff == 0))
        return;

    if (E.curx - E.coloff == 0) {

        if (E.coloff > 0) {
            scrollLeft();
        } else {
            if (E.cury - E.rowoff == 0)
                scrollUp();
            else
                E.cury--;

            E.curx = E.rows[E.cury].len;
            if (E.curx > maxcols) {
                E.coloff = E.curx - maxcols + 1;
                displayScreen();
            }
        }

    } else {
        E.curx--;
    }
    E.scurx = E.curx;

    moveCursor();
}

void rightArrow(void) {

    if ((E.curx == maxcols-1) && (E.cury == E.numrows-1))
        return;

    if (E.curx >= E.rows[E.cury].len) {

        if (E.cury - E.rowoff == maxrows-1)
            scrollDown();

        E.cury++;
        E.curx = 0;

        if (E.coloff > 0) {
            E.coloff = 0;
            displayScreen();
        }

    } else if (E.curx - E.coloff == maxcols-1) {

        scrollRight();
        E.curx++;

    } else {

        E.curx++;
    }
    E.scurx = E.curx;

    moveCursor();
}

void upArrow(void) {

    if (E.cury == 0)
        return;

    if (E.cury - E.rowoff == 0)
        scrollUp();

    E.cury--;

    unsigned int len = E.rows[E.cury].len;
    E.curx = len < E.scurx ? len : E.scurx;
    if (E.coloff != 0) {
        E.coloff = 0;
        displayScreen();
    }

    moveCursor();
}

void downArrow(void) {

    if (E.cury == E.numrows-1)
        return;

    if (E.cury - E.rowoff == maxrows-1)
        scrollDown();

    E.cury++;

    unsigned int len = E.rows[E.cury].len;

    E.curx = len < E.scurx ? len : E.scurx;
    if (E.curx > maxcols) {
        E.coloff = E.curx - maxcols;
        E.curx = maxcols;
        displayScreen();
    } else {
        E.coloff = 0;
    }

    moveCursor();
}

void escapeKey(void) {

    state = NORMAL;
    updateTopBar();
    moveCursor();
}

void startOfLine(void) {

    E.scurx = 0;
    E.curx = 0;
    E.coloff = 0;

    displayScreen();
    moveCursor();
}


bool executeMapping(mapping_t* mapping, unsigned int size, int c) {

    for (unsigned int i = 0; i < size/sizeof(mapping_t); i++) {

        if (mapping[i].key == c) {
            mapping[i].handler();
            return true;
        }
    }

    return false;
}





mapping_t insertMapping[] = {
    {'\e', escapeKey},
    {KEY_LEFT, leftArrow},
    {KEY_RIGHT, rightArrow},
    {KEY_UP, upArrow},
    {KEY_DOWN, downArrow},
};

mapping_t normalMapping[] = {
    {'\e', escapeKey},
    {KEY_LEFT, leftArrow},
    {KEY_RIGHT, rightArrow},
    {KEY_UP, upArrow},
    {KEY_DOWN, downArrow},
    {'h', leftArrow},
    {'l', rightArrow},
    {'k', upArrow},
    {'j', downArrow},
    {'0', startOfLine},
};





void main(int argc, char* argv[]) {

    if (argc != 2)
        return;

    initscr();
    noecho();

    maxcols = getmaxx(stdscr);
    maxrows = getmaxy(stdscr);

    topBar = newwin(1, maxcols, maxrows-2, 0);
    cmdBar = newwin(1, maxcols, maxrows-1, 0);
    maxrows -= 2;
    refresh();

    E.rows = NULL;
    E.numrows = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.scurx = 0;
    E.curx = 0;
    E.cury = 0;

    currentFile = argv[1];
    readFile(currentFile);

    char cmdBuffer[MAX_CMD_BUFFER+1];
    unsigned int cmdCursor = 0;
    unsigned int cmdSize = 0;
    int ch;

    state = NORMAL;

    displayScreen();
    moveCursor();

    while (true) {

        ch = getch();
        if (state == INSERT) {

            if (ch == '\n') {
                E.curx = 0;
                E.cury++;
                moveCursor();
            }
            else if ((ch == KEY_BACKSPACE) || (ch == '\b')) {
                if (E.curx == 0) {

                    if (E.cury == 0)
                        continue;

                    //E.curx = line.linelen;
                    E.cury--;

                } else {

                    E.curx--;
                }
                moveCursor();
            }
            else {

                if (executeMapping(insertMapping, sizeof(insertMapping), ch))
                    continue;

                moveCursor();
                printw("%c", ch);
                wrefresh(stdscr);

                E.curx++;
                if (E.curx >= maxcols) {
                    E.curx = 0;
                    E.cury++;
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
            else {
                executeMapping(normalMapping, sizeof(normalMapping), ch);
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
                clearCmdBar();
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

