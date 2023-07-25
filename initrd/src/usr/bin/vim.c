
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>




#define TAB_RENDER "    "
#define TAB_SIZE   (sizeof(TAB_RENDER)-1)



#define MAX_CMD_BUFFER 32

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef enum {
    NORMAL,
    COMMAND,
    INSERT,
} state_t;


WINDOW* topBar;
WINDOW* cmdBar;

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
    unsigned int rlen;
    char* chars;
    char* rchars;
} row_t;

struct {
    row_t* rows;
    unsigned int numrows;
    unsigned int rowoff;
    unsigned int coloff;
    unsigned int scurx;
    unsigned int curx;
    unsigned int cury;
    state_t state;
} E;


/*
void renderRow(row_t* row) {

    // Incase there is something there
    free(row->rchars);

    row->rlen = 0;
    row->rchars = (char*) malloc(1);
    for (unsigned int i = 0; i < row->len; i++) {
        char c = row->chars[i];
        if (c == '\t') {
            row->rchars = realloc(row->rchars, row->rlen + TAB_SIZE);
            memcpy(row->rchars + row->rlen, TAB_RENDER, TAB_SIZE);
            row->rlen += TAB_SIZE;
        }
        if (c == '\e') {
            row->rchars = realloc(row->rchars, row->rlen + 1);
            memcpy(row->rchars + row->rlen, "^[", 2);
            row->rlen += 2;
        }
        else {
            row->rchars = realloc(row->rchars, row->rlen + 1);
            row->rchars[row->rlen++] = c;
        }
    }
    row->rchars = realloc(row->rchars, row->rlen + 1);
    row->rchars[row->rlen] = '\0';
}
*/

void appendRow(char* s, unsigned int linelen) {

    // If maxrows is NULL then realloc will call malloc for us
    printf("Before\n");
    E.rows = realloc(E.rows, sizeof(row_t) * (E.numrows + 1));

    row_t* row = &E.rows[E.numrows];
    printf("Row at 0x%x\n", row);
    row->len = linelen;
    row->chars = (char*) malloc(linelen + 1);
    memcpy(row->chars, s, linelen+1);

    //row->rchars = NULL;
    //renderRow(row);

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

        printf("Appending '%s' with len %d\n", line, linelen);
        appendRow(line, linelen);
    }
    for (;;) {}

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

    mvwprintw(topBar, 0, 0, "%s  %s", stateToString(E.state), currentFile);
    wclrtoeol(topBar);
}

void parseCommand(char* buf) {

    if (strcmp(buf, "q") == 0) {
        endwin();
        exit(0);
    }
    else {
        werase(cmdBar);
        mvwprintw(cmdBar, 0, 0, "Not an editor command: %s", buf);
    }
}

void displayScreen(void) {

    move(0, 0);

    char* buf = NULL;
    size_t i = 0;
    for (i = 0; (i < maxrows) && (i < E.numrows-E.rowoff); i++) {

        unsigned int len = E.rows[i + E.rowoff].len;
        if (len <= E.coloff) {
            clrtoeol();
            printw("\n");
            continue;
        }

        len -= E.coloff;
        len = len > maxcols ? maxcols : len;

        buf = realloc(buf, len+1);
        memcpy(buf, E.rows[i + E.rowoff].chars + E.coloff, len);
        buf[len] = '\0';

        printw("%s", buf);
        clrtoeol();
        printw("\n");
    }
    free(buf);

    // Draw '~' for empty lines
    for (; i < maxrows; i++) {

        printw("~");
        clrtoeol();
        if (i != maxrows-1)
            printw("\n");
    }
}

void scrollUp(void) {

    if (E.rowoff == 0)
        return;

    E.rowoff--;
}

void scrollDown(void) {

    if (E.rowoff + maxrows == E.numrows)
        return;

    E.rowoff++;
}

void scrollLeft(void) {

    E.coloff--;
}

void scrollRight(void) {

    E.coloff++;
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
                E.coloff = E.curx - maxcols + maxcols/2;
            }
        }

    } else {
        E.curx--;
    }
    E.scurx = E.curx;
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
        }

    } else if (E.curx - E.coloff == maxcols-1) {

        scrollRight();
        E.curx++;

    } else {

        E.curx++;
    }
    E.scurx = E.curx;
}

void upArrow(void) {

    if (E.cury == 0)
        return;

    if (E.cury - E.rowoff == 0)
        scrollUp();

    E.cury--;

    unsigned int len = E.rows[E.cury].len;

    E.curx = MIN(len, E.scurx);
    if (E.curx < maxcols-1)
        E.coloff = 0;
    else
        E.coloff = (E.curx+maxcols > E.coloff || E.curx < E.coloff) ? (E.curx-maxcols+maxcols/2) : (E.coloff);
}

void downArrow(void) {

    if (E.cury == E.numrows-1)
        return;

    if (E.cury - E.rowoff == maxrows-1)
        scrollDown();

    E.cury++;

    unsigned int len = E.rows[E.cury].len;

    E.curx = MIN(len, E.scurx);
    if (E.curx < maxcols-1)
        E.coloff = 0;
    else
        E.coloff = (E.curx+maxcols > E.coloff || E.curx < E.coloff) ? (E.curx-maxcols+maxcols/2) : (E.coloff);
}

void escapeKey(void) {

    E.state = NORMAL;
}

void startOfLine(void) {

    E.scurx = 0;
    E.curx = 0;
    E.coloff = 0;
}

void endOfLine(void) {

    E.scurx = E.rows[E.cury].len;
    E.curx = E.scurx;
    E.coloff = E.scurx > maxcols ? E.scurx-maxcols+maxcols/2 : 0;
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
    {'$', endOfLine},
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

    E.state = NORMAL;

    clear();
    refresh();
    while (true) {

        displayScreen();
        updateTopBar();

        wrefresh(cmdBar);
        wrefresh(topBar);
        refresh();
        moveCursor();

        ch = getch();
        if (E.state == INSERT) {

            if (ch == '\n') {
                E.curx = 0;
                E.cury++;
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
            }
            else {

                if (executeMapping(insertMapping, sizeof(insertMapping), ch))
                    continue;

                moveCursor();
                printw("%c", ch);

                E.curx++;
                if (E.curx >= maxcols) {
                    E.curx = 0;
                    E.cury++;
                }
            }
        }
        else if (E.state == NORMAL) {

            if (ch == ':') {
                E.state = COMMAND;
                cmdCursor = 0;
                mvwprintw(cmdBar, 0, 0, ":");
                wclrtoeol(cmdBar);
            }
            else if (ch == 'i') {
                E.state = INSERT;
            }
            else {
                executeMapping(normalMapping, sizeof(normalMapping), ch);
            }
        }
        else if (E.state == COMMAND) {

            if (cmdCursor > MAX_CMD_BUFFER) {
                wprintw(cmdBar, "Max command buffer size reached\n");
                break;
            }

            if (ch == '\e') {
                E.state = NORMAL;
                werase(cmdBar);
            }
            else if (ch == '\n') {
                E.state = NORMAL;
                cmdBuffer[cmdSize] = '\0';
                parseCommand(cmdBuffer);
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
                cmdSize--;
            }
            else if (ch == 127) {
                wdelch(cmdBar);
                cmdSize--;
            }
            else {

                if (cmdCursor == cmdSize) {
                    cmdBuffer[cmdCursor++] = ch;
                    cmdSize++;
                    wprintw(cmdBar, "%c", ch);
                    wclrtoeol(cmdBar);
                }
                else {
                    memmove(&cmdBuffer[cmdCursor+1], &cmdBuffer[cmdCursor], cmdSize-cmdCursor);
                    cmdBuffer[cmdCursor++] = ch;
                    cmdBuffer[++cmdSize] = '\0';
                    werase(cmdBar);
                    mvwprintw(cmdBar, 0, 0, ":%s", cmdBuffer);
                    wmove(cmdBar, 0, cmdCursor+1);
                }

            }
        }
    }


    delwin(topBar);
    delwin(cmdBar);
    endwin();
}

