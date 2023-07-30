
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>




#define TAB_RENDER   "    "
#define TAB_AS_SPACE 0
#define TOP_MARGIN   3



#define TAB_SIZE   (sizeof(TAB_RENDER)-1)
#define MAX_CMD_BUFFER 32

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

typedef enum {
    NORMAL,
    COMMAND,
    INSERT,
    VISUAL,
    VISUAL_LINE
} mode_t;

WINDOW* topBar;
WINDOW* cmdBar;

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
    unsigned int rscurx;
    unsigned int rcurx;
    unsigned int scurx;
    unsigned int curx;

    unsigned int rvcurx;
    unsigned int vcurx;
    unsigned int vcury;

    unsigned int cury;
    mode_t mode;

    unsigned int clipSize;
    char* clipboard;
    char* filename;
    bool saved;
} E;


extern inline row_t* currentRow(void) {
    return &E.rows[E.cury];
}

extern inline void snapCursor(void) {
//void snapCursor(void) {

    unsigned int len = currentRow()->rlen;
    unsigned int max = MIN(len, E.rscurx);

#if TAB_AS_SPACE
    E.rcurx = max;
    E.curx = max;
#else
    E.curx = 0;
    E.rcurx = 0;
    for (; E.rcurx < max; E.curx++) {
        char c = currentRow()->chars[E.curx];
        if (c == '\t')
            E.rcurx += TAB_SIZE - (E.rcurx % TAB_SIZE);
        else if (c == '\e')
            E.rcurx += 2;
        else
            E.rcurx++;
    }
#endif
}

extern inline unsigned int curxToRCurx(unsigned int curx) {

    unsigned int rcurx = 0;
    for (unsigned int i = 0; i < curx; i++) {
        char c = currentRow()->chars[i];
        if (c == '\t')
            rcurx += TAB_SIZE - (rcurx % TAB_SIZE);
        else if (c == '\e')
            rcurx += 2;
        else
            rcurx++;
    }

    return rcurx;
}

void renderRow(row_t* row) {

    // Incase there is something there
    free(row->rchars);

    row->rlen = 0;
    row->rchars = NULL;
    for (unsigned int i = 0; i < row->len; i++) {
        char c = row->chars[i];
        if (c == '\t') {

            unsigned int size = TAB_SIZE - (row->rlen % TAB_SIZE);

            row->rchars = realloc(row->rchars, row->rlen + size);
            memcpy(row->rchars + row->rlen, TAB_RENDER, size);
            row->rlen += size;
        }
        else if (c == '\e') {
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

void insertCharacter(row_t* row, unsigned int at, int ch) {

    row->chars = (char*) realloc(row->chars, row->len+2);
    memmove(&row->chars[at+1], &row->chars[at], row->len - at + 1);
    row->chars[at] = (char) ch;
    row->len++;
    renderRow(row);
}

void appendRow(char* s, unsigned int linelen) {

    // If maxrows is NULL then realloc will call malloc for us
    E.rows = (row_t*) realloc(E.rows, sizeof(row_t) * (E.numrows + 1));

    row_t* row = &E.rows[E.numrows];
    row->len = linelen;
    row->chars = (char*) malloc(linelen + 1);
    memcpy(row->chars, s, linelen+1);

    row->rchars = NULL;
    renderRow(row);

    E.numrows++;
}

void readFile(char* filename) {

    free(E.filename);

    unsigned int fnlen = strlen(filename);
    E.filename = malloc(fnlen + 1);
    memcpy(E.filename, filename, fnlen + 1);

    //int fd = open(filename, O_RDONLY | O_CREAT, 0664);
    int fd = open(filename, O_RDONLY, 0664);
    if (fd == -1) {
        int backup = errno;

        // Don't exit because file didn't exist
        if (backup == ENOENT) {
            E.saved = false;
            return;
        }

        printf("open error: %s\n", strerror(backup));

        delwin(topBar);
        delwin(cmdBar);
        endwin();
        exit(1);
    }

    E.saved = true;

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

    free(line);

    if (fclose(fp) == -1) {
        int backup = errno;
        printf("fclose error: %s\n", strerror(backup));
        exit(1);
    }
}

char* stateToString(mode_t st) {
    if      (st == NORMAL)       return "-- NORMAL --";
    else if (st == COMMAND)      return "-- COMMAND --";
    else if (st == INSERT)       return "-- INSERT --";
    else if (st == VISUAL)       return "-- VISUAL --";
    else if (st == VISUAL_LINE)  return "-- V-LINE --";
    else                         return "-- UNKNOWN --";
}

void updateTopBar(void) {

    mvwprintw(topBar, 0, 0, "%s\t%s%s\t%d/%d",
              stateToString(E.mode),
              E.filename ? E.filename : "[Empty]",
              E.saved ? "" : " (*)",
              E.cury+1,
              E.numrows);

    wclrtoeol(topBar);
}

unsigned int saveFile(char* filename) {

    int fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0664);
    if (fd == -1) {
        int backup = errno;
        printf("open error: %s\n", strerror(backup));

        delwin(topBar);
        delwin(cmdBar);
        endwin();
        exit(1);
    }

    E.saved = true;

    unsigned int bytesWritten = 0;
    for (unsigned int i = 0; i < E.numrows; i++) {
        bytesWritten += write(fd, E.rows[i].chars, E.rows[i].len);
        bytesWritten += write(fd, "\n", 1);
    }

    if (close(fd) == -1) {
        int backup = errno;
        printf("close error: %s\n", strerror(backup));
        exit(1);
    }

    return bytesWritten;
}

void parseCommand(char* buf) {

    if (strcmp(buf, "q") == 0) {

        if (!E.saved) {
            mvwprintw(cmdBar, 0, 0, "No write since last change");
            wclrtoeol(cmdBar);
            return;
        }

        delwin(topBar);
        delwin(cmdBar);
        endwin();
        exit(0);
    }
    if (strcmp(buf, "q!") == 0) {

        delwin(topBar);
        delwin(cmdBar);
        endwin();
        exit(0);
    }
    else if (strcmp(buf, "w") == 0) {

        if (!E.filename) {
            mvwprintw(cmdBar, 0, 0, "No filename");
            wclrtoeol(cmdBar);
            return;
        }

        unsigned int bytesWritten = saveFile(E.filename);
        mvwprintw(cmdBar, 0, 0, "\"%s\" %dL, %dB written", E.filename, E.numrows, bytesWritten);
        wclrtoeol(cmdBar);
    }
    else if (strncmp(buf, "w ", 2) == 0) {

        char* filename = buf + 2;
        if (*filename == '\0') {
            mvwprintw(cmdBar, 0, 0, "No filename");
            wclrtoeol(cmdBar);
            return;
        }

        if (!E.filename) {
            //free(E.filename);
            unsigned int fnlen = strlen(filename);
            E.filename = malloc(fnlen + 1);
            memcpy(E.filename, filename, fnlen + 1);
        }

        unsigned int bytesWritten = saveFile(filename);
        mvwprintw(cmdBar, 0, 0, "\"%s\" %dL, %dB written", filename, E.numrows, bytesWritten);
        wclrtoeol(cmdBar);
    }
    else if (strcmp(buf, "wq") == 0) {
        if (!E.filename) {
            mvwprintw(cmdBar, 0, 0, "No filename");
            wclrtoeol(cmdBar);
            return;
        }

        unsigned int bytesWritten = saveFile(E.filename);
        mvwprintw(cmdBar, 0, 0, "\"%s\" %dL, %dB written", E.filename, E.numrows, bytesWritten);

        delwin(topBar);
        delwin(cmdBar);
        endwin();
        exit(0);
    }
    else if (strncmp(buf, "wq ", 3) == 0) {

        char* filename = buf + 3;
        if (*filename == '\0') {
            mvwprintw(cmdBar, 0, 0, "No filename");
            wclrtoeol(cmdBar);
            return;
        }

        saveFile(filename);
        delwin(topBar);
        delwin(cmdBar);
        endwin();
        exit(0);
    }
    else {
        mvwprintw(cmdBar, 0, 0, "Not an editor command: %s", buf);
        wclrtoeol(cmdBar);
    }
}

void displayScreen(void) {

    move(0, 0);

    unsigned int starty = MIN(E.cury, E.vcury);
    unsigned int endy = MAX(E.cury, E.vcury);

    char* buf = NULL;
    size_t i;
    for (i = 0; (i < maxrows) && (i < E.numrows-E.rowoff); i++) {

        unsigned int len = E.rows[i + E.rowoff].rlen;
        if (len <= E.coloff) {
            clrtoeol();
            printw("\n");
            continue;
        }

        len -= E.coloff;
        len = len > maxcols ? maxcols : len;

        buf = realloc(buf, len+1);
        if ((E.mode == VISUAL) && (starty - E.rowoff <= i) && (i <= endy - E.rowoff)) {

            // If the selection is on a single line
            if ((i == starty - E.rowoff) && (i == endy - E.rowoff)) {

                unsigned int startx = MIN(E.curx, E.vcurx);
                unsigned int endx = MAX(E.curx, E.vcurx);

                attron(COLOR_PAIR(1));
                if (startx > 0) {
                    memcpy(buf, E.rows[i + E.rowoff].rchars + E.coloff, startx);
                    buf[startx] = '\0';
                    printw("%s", buf);
                }
                attroff(COLOR_PAIR(1));

                attron(COLOR_PAIR(4));
                memcpy(buf, E.rows[i + E.rowoff].rchars + E.coloff + startx, endx - startx);
                buf[endx - startx] = '\0';
                printw("%s", buf);
                attroff(COLOR_PAIR(4));

                if (len + 1 > endx) {
                    attron(COLOR_PAIR(1));
                    memcpy(buf, E.rows[i + E.rowoff].rchars + E.coloff + endx, len-endx);
                    buf[len-endx] = '\0';
                    printw("%s", buf);
                    attroff(COLOR_PAIR(1));
                }
            }
            // If this row is where the cursor is
            else if (i == E.cury - E.rowoff) {

                short c1 = E.cury <= E.vcury ? 1 : 4;
                short c2 = E.cury <= E.vcury ? 4 : 1;
                if (E.curx <= len) {
                    attron(COLOR_PAIR(c1));
                    memcpy(buf, E.rows[i + E.rowoff].rchars + E.coloff, E.curx);
                    buf[E.curx] = '\0';
                    printw("%s", buf);
                    attroff(COLOR_PAIR(c1));
                }

                attron(COLOR_PAIR(c2));
                memcpy(buf, E.rows[i + E.rowoff].rchars + E.coloff + E.curx, len-E.curx);
                buf[len-E.curx] = '\0';
                printw("%s", buf);
                attroff(COLOR_PAIR(c2));
            }
            // If this is the row where selection starts
            else if (i == starty - E.rowoff) {
                attron(COLOR_PAIR(1));
                if (E.vcurx > 0) {
                    memcpy(buf, E.rows[i + E.rowoff].rchars + E.coloff, E.vcurx);
                    buf[E.vcurx] = '\0';
                    printw("%s", buf);
                }
                attroff(COLOR_PAIR(1));

                attron(COLOR_PAIR(4));
                memcpy(buf, E.rows[i + E.rowoff].rchars + E.coloff + E.vcurx, len-E.vcurx);
                buf[len-E.vcurx] = '\0';
                printw("%s", buf);
                attroff(COLOR_PAIR(4));
            }
            // If this row is in the middle of the selection
            else {
                attron(COLOR_PAIR(4));
                memcpy(buf, E.rows[i + E.rowoff].rchars + E.coloff, len);
                buf[len] = '\0';
                printw("%s", buf);

                attroff(COLOR_PAIR(4));
            }
        }
        else if ((E.mode == VISUAL_LINE) && (starty - E.rowoff <= i) && (i <= endy - E.rowoff)) {

            attron(COLOR_PAIR(4));
            memcpy(buf, E.rows[i + E.rowoff].rchars + E.coloff, len);
            buf[len] = '\0';
            printw("%s", buf);

            attroff(COLOR_PAIR(4));
        }
        else {

            attron(COLOR_PAIR(1));
            memcpy(buf, E.rows[i + E.rowoff].rchars + E.coloff, len);
            buf[len] = '\0';
            printw("%s", buf);
            attroff(COLOR_PAIR(1));
        }

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
    move(E.cury - E.rowoff, E.rcurx - E.coloff);
}



void leftArrow(void) {

    if ((E.rcurx - E.coloff == 0) && (E.cury - E.rowoff == 0))
        return;

    if (E.rcurx - E.coloff == 0) {

        if (E.coloff > 0) {
            scrollLeft();
        } else {
            if (E.cury - E.rowoff == 0)
                scrollUp();
            else
                E.cury--;

            E.rcurx = currentRow()->rlen;
            E.curx = currentRow()->len;
            if (E.rcurx > maxcols) {
                E.coloff = E.rcurx - maxcols + maxcols/2;
            }
        }

    } else {

        E.curx--;

#if TAB_AS_SPACE
        E.rcurx--;
#else
        if (currentRow()->chars[E.curx] == '\t')
            E.rcurx = curxToRCurx(E.curx);
        else if (currentRow()->chars[E.curx] == '\e')
            E.rcurx -= 2;
        else
            E.rcurx--;
#endif

    }
    E.rscurx = E.rcurx;
    E.scurx = E.curx;
}

void rightArrow(void) {

    if ((E.numrows == 0) || ((E.cury >= E.numrows-1)))
        return;

    if (E.rcurx >= currentRow()->rlen) {

        if (E.cury - E.rowoff == maxrows-1)
            scrollDown();

        E.cury++;
        E.rcurx = 0;
        E.curx = 0;

        if (E.coloff > 0) {
            E.coloff = 0;
        }

    } else if (E.rcurx - E.coloff == maxcols-1) {

        scrollRight();

#if TAB_AS_SPACE
        E.rcurx++;
#else
        if (currentRow()->chars[E.curx] == '\t')
            E.rcurx += TAB_SIZE - (E.rcurx % TAB_SIZE);
        else if (currentRow()->chars[E.curx] == '\e')
            E.rcurx += 2;
        else
            E.rcurx++;

#endif

    E.curx++;

    } else {

#if TAB_AS_SPACE
        E.rcurx++;
#else
        if (currentRow()->chars[E.curx] == '\t')
            E.rcurx += TAB_SIZE - (E.rcurx % TAB_SIZE);
        else if (currentRow()->chars[E.curx] == '\e')
            E.rcurx += 2;
        else
            E.rcurx++;
#endif

        E.curx++;
    }
    E.rscurx = E.rcurx;
    E.scurx = E.curx;
}

void upArrow(void) {

    if (E.cury == 0)
        return;

    if (E.cury - E.rowoff == 0)
        scrollUp();

    E.cury--;

    snapCursor();

    if (E.rcurx < maxcols-1)
        E.coloff = 0;
    else
        E.coloff = (E.rcurx+maxcols > E.coloff || E.rcurx < E.coloff) ? (E.rcurx-maxcols+maxcols/2) : (E.coloff);
}

void downArrow(void) {

    if ((E.numrows == 0) || (E.cury == E.numrows-1))
        return;

    if (E.cury - E.rowoff == maxrows-1)
        scrollDown();

    E.cury++;

    snapCursor();

    if (E.rcurx < maxcols-1)
        E.coloff = 0;
    else
        E.coloff = (E.rcurx+maxcols > E.coloff || E.rcurx < E.coloff) ? (E.rcurx-maxcols+maxcols/2) : (E.coloff);
}

void escapeKey(void) {

    E.mode = NORMAL;
}

void startOfLine(void) {

    E.rscurx = 0;
    E.rcurx = 0;
    E.scurx = 0;
    E.curx = 0;

    E.coloff = 0;
}

void gotoEndOfLine(void) {

    E.rscurx = currentRow()->rlen;
    E.rcurx = E.rscurx;
    E.scurx = currentRow()->len;
    E.curx = E.scurx;

    E.coloff = E.rscurx > maxcols ? E.rscurx-maxcols+maxcols/2 : 0;
}

void gotoStartOfFile(void) {

    E.rscurx = 0;
    E.rcurx = 0;
    E.scurx = 0;
    E.curx = 0;

    E.cury = 0;
    E.rowoff = 0;
    E.coloff = 0;
}

void gotoEndOfFile(void) {

    E.rscurx = 0;
    E.rcurx = 0;
    E.scurx = 0;
    E.curx = 0;

    E.cury = E.numrows-1;
    E.rowoff = E.numrows-maxrows;
    E.coloff = 0;
}

void splitCurrentRow(void) {

    E.rows = (row_t*) realloc(E.rows, sizeof(row_t) * (E.numrows + 1));
    memmove(&E.rows[E.cury+1], &E.rows[E.cury], sizeof(row_t) * (E.numrows - E.cury));
    row_t* frow = &E.rows[E.cury];
    row_t* trow = &E.rows[E.cury + 1];

    trow->len = frow->len - E.curx;
    trow->chars = (char*) malloc(frow->len - E.curx + 1);
    trow->rchars = NULL;

    memmove(trow->chars, &frow->chars[E.curx], frow->len - E.curx);
    trow->chars[frow->len - E.curx] = '\0';

    frow->chars = realloc(frow->chars, E.curx + 1);
    frow->chars[E.curx] = '\0';
    frow->len = E.curx;

    renderRow(frow);
    renderRow(trow);

    E.numrows++;
    E.cury++;
    E.rscurx = 0;
    E.scurx = 0;
    E.rcurx = 0;
    E.curx = 0;
    E.coloff = 0;
}

void deleteCurrentChar(void) {

    if (E.curx == 0) {
        if (E.cury == 0)
            return;

        row_t* trow = &E.rows[E.cury-1];
        row_t* frow = &E.rows[E.cury];

        if (E.coloff > 0)
            scrollLeft();

        E.rscurx = trow->rlen;
        E.scurx = trow->len;
        E.rcurx = trow->rlen;
        E.curx = trow->len;
        if (E.rcurx > maxcols) {
            E.coloff = E.rcurx - maxcols + maxcols/2;
        }

        trow->chars = (char*) realloc(trow->chars, trow->len + frow->len + 1);
        memcpy(&trow->chars[trow->len], frow->chars, frow->len+1);
        trow->len += frow->len;

        renderRow(trow);

        if (E.cury < E.numrows-1)
            memmove(&E.rows[E.cury], &E.rows[E.cury+1], sizeof(row_t) * (E.numrows - E.cury - 1));
        E.rows = (row_t*) realloc(E.rows, sizeof(row_t) * (E.numrows-1));
        E.numrows--;

        if ((E.rowoff > 0) && (E.cury - E.rowoff < TOP_MARGIN))
            E.rowoff--;

        E.cury--;

        return;
    }

    row_t* row = &E.rows[E.cury];
    row->chars = (char*) realloc(row->chars, row->len);
    memmove(&row->chars[E.curx-1], &row->chars[E.curx], row->len - E.curx);
    row->chars[--row->len] = '\0';

    renderRow(row);

    E.curx--;
    if (E.coloff > 0)
        scrollLeft();

    if (row->chars[E.curx] == '\t')
        E.rcurx = curxToRCurx(E.curx);
    else if (row->chars[E.curx] == '\e')
        E.rcurx -= 2;
    else
        E.rcurx--;

    E.rscurx = E.rcurx;
    E.scurx = E.curx;
}

void deleteCurrentLine(void) {

    if (E.numrows == 0)
        return;

    if (E.cury < E.numrows-1)
        memmove(&E.rows[E.cury], &E.rows[E.cury+1], sizeof(row_t) * (E.numrows - E.cury - 1));

    E.rows = (row_t*) realloc(E.rows, sizeof(row_t) * (E.numrows-1));
    E.numrows--;

    snapCursor();
}

void saveSelection(void) {

    unsigned int starty = MIN(E.cury, E.vcury);
    unsigned int endy = MAX(E.cury, E.vcury);

    if (E.mode == VISUAL) {

    }
    else if (E.mode == VISUAL_LINE) {

        E.clipSize = 0;

        for (unsigned int y = starty; y <= endy; y++) {

            E.clipboard = (char*) realloc(E.clipboard, E.clipSize + E.rows[y].len + 1);

            E.clipboard[E.clipSize] = '\n';
            memcpy(&E.clipboard[E.clipSize + 1], E.rows[y].chars, E.rows[y].len);
            E.clipSize += E.rows[y].len + 1;
        }

        E.clipboard = (char*) realloc(E.clipboard, E.clipSize + 1);
        E.clipboard[E.clipSize] = '\0';
    }

    E.mode = NORMAL;
}

void pasteClipboard(void) {

    /*
    move(0, 0);
    clear();
    printf("%s", E.clipboard);
    for (;;) {}
    */

    if ((E.clipboard == NULL) || (E.clipSize == 0))
        return;

    char c;
    row_t* row = NULL;
    for (unsigned int i = 0; (c = E.clipboard[i]) && i < E.clipSize; i++) {

        bool foundNewline = true;
        unsigned int end;
        for (end = i; E.clipboard[end] != '\n'; end++) {
            if (end == E.clipSize) {
                foundNewline = false;
                break;
            }
        }

        row = &E.rows[E.cury];

        unsigned int len = end - i;
        if (len > 0) {
            row->chars = (char*) realloc(row->chars, row->len + len);
            memcpy(&row->chars[row->len], &E.clipboard[i], len);
            row->len += len;
        }

        if (foundNewline) {

            row->chars = (char*) realloc(row->chars, row->len + 1);
            row->chars[row->len] = '\0';
            renderRow(row);

            E.cury++;
            if (E.cury - E.rowoff > maxrows)
                E.rowoff++;

            E.rows = (row_t*) realloc(E.rows, sizeof(row_t) * (E.numrows + 1));
            memmove(&E.rows[E.cury+1], &E.rows[E.cury], sizeof(row_t) * (E.numrows - E.cury));
            E.numrows++;

            row_t* r = &E.rows[E.cury];
            r->chars = NULL;
            r->len = 0;
        }

        i = end;
    }

    if (row == NULL)
        return;

    row->chars = (char*) realloc(row->chars, row->len + 1);
    row->chars[row->len] = '\0';

    renderRow(row);
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
    {'\n', splitCurrentRow},
    {'\b', deleteCurrentChar}
    //{127, deletePreviousChar}
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
    {'$', gotoEndOfLine},
    {'g', gotoStartOfFile},
    {'G', gotoEndOfFile},
    {'d', deleteCurrentLine},
    {'p', pasteClipboard},
};

mapping_t visualMapping[] = {
    {'\e', escapeKey},
    {'v', escapeKey},
    {KEY_LEFT, leftArrow},
    {KEY_RIGHT, rightArrow},
    {KEY_UP, upArrow},
    {KEY_DOWN, downArrow},
    {'h', leftArrow},
    {'l', rightArrow},
    {'k', upArrow},
    {'j', downArrow},
    {'0', startOfLine},
    {'$', gotoEndOfLine},
    {'g', gotoStartOfFile},
    {'G', gotoEndOfFile},
    {'y', saveSelection},
    {'p', pasteClipboard},
};




void main(int argc, char* argv[]) {

    initscr();
    start_color();
    noecho();

    E.rows = NULL;
    E.numrows = 0;
    E.rowoff = 0;
    E.coloff = 0;

    E.rscurx = 0;
    E.rcurx = 0;
    E.scurx = 0;
    E.curx = 0;
    E.cury = 0;

    E.clipSize = 0;
    E.clipboard = NULL;
    E.filename = NULL;
    E.saved = false;


    // Main color
    init_pair(1, COLOR_WHITE, COLOR_BLACK);

    // Top-bar color
    init_pair(2, COLOR_BLACK, COLOR_WHITE);

    // Cmd-bar color
    init_pair(3, COLOR_WHITE, COLOR_BLACK);

    // Visual-select color
    init_pair(4, COLOR_BLACK, COLOR_WHITE);

    maxcols = getmaxx(stdscr);
    maxrows = getmaxy(stdscr);

    topBar = newwin(1, maxcols, maxrows-2, 0);
    cmdBar = newwin(1, maxcols, maxrows-1, 0);
    maxrows -= 2;
    refresh();


    if (argc > 1)
        readFile(argv[1]);

    if (E.numrows == 0)
        appendRow("", 0);

    char cmdBuffer[MAX_CMD_BUFFER+1];
    unsigned int cmdCursor = 0;
    unsigned int cmdSize = 0;
    int ch;

    E.mode = NORMAL;

    //clear();
    //refresh();
    attron(COLOR_PAIR(1));
    wattron(topBar, COLOR_PAIR(2));
    wattron(cmdBar, COLOR_PAIR(1));
    while (true) {

        displayScreen();
        updateTopBar();

        refresh();
        wrefresh(topBar);
        wrefresh(cmdBar);
        moveCursor();

        ch = getch();
        if (E.mode == INSERT) {

            if (executeMapping(insertMapping, sizeof(insertMapping), ch))
                continue;

            insertCharacter(&E.rows[E.cury], E.curx, ch);
            E.saved = false;
            rightArrow();
        }
        else if (E.mode == NORMAL) {

            if (ch == ':') {
                E.mode = COMMAND;
                cmdCursor = 0;
                mvwprintw(cmdBar, 0, 0, ":");
                wclrtoeol(cmdBar);
            }
            else if (ch == 'i') {
                E.mode = INSERT;
            }
            else if (ch == 'v') {
                E.mode = VISUAL;
                E.rvcurx = E.rcurx;
                E.vcurx = E.curx;
                E.vcury = E.cury;
            }
            else if (ch == 'V') {
                E.mode = VISUAL_LINE;
                E.vcury = E.cury;
            }
            else {
                executeMapping(normalMapping, sizeof(normalMapping), ch);
            }
        }
        else if (E.mode == VISUAL) {

            executeMapping(visualMapping, sizeof(visualMapping), ch);
        }
        else if (E.mode == VISUAL_LINE) {

            executeMapping(visualMapping, sizeof(visualMapping), ch);
        }
        else if (E.mode == COMMAND) {

            if (cmdCursor > MAX_CMD_BUFFER) {
                mvwprintw(cmdBar, 0, 0, "Max command buffer size reached");
                wclrtoeol(cmdBar);
                E.mode = NORMAL;
                continue;
            }

            if (ch == '\e') {
                E.mode = NORMAL;
                werase(cmdBar);
            }
            else if (ch == '\n') {
                E.mode = NORMAL;
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
                    mvwprintw(cmdBar, 0, 0, ":%s", cmdBuffer);
                    wclrtoeol(cmdBar);
                    wmove(cmdBar, 0, cmdCursor+1);
                }

            }
        }
    }


    delwin(topBar);
    delwin(cmdBar);
    endwin();
}

