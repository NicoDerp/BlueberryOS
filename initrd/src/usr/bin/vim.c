
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <ctype.h>




/* What you wan't tabs to render as (default is 4 spaces) */
#define TAB_RENDER        "    "

/* 0 then tabs are tabs, 1 for tabs are spaces (bit buggy) */
#define TAB_AS_SPACE      0

/* 1 for line numbers */
#define LINE_NUMBERS      1


/* Characters of margin left and right when scrolling */
#define SCROLL_MARGIN_S   8

/* Lines of margin top and bottom when scrolling */
#define SCROLL_MARGIN_TB  2


/* Characters of margin left and right when searching */
#define SEARCH_MARGIN_S   8

/* Lines of margin top and bottom when searching */
#define SEARCH_MARGIN_TB  4


/* Characters of margin left at all times */
#define LEFT_MARGIN       1

/* Characters of margin between the line numbers and text. Only used when LINE_NUMBERS is 1 */
#define LINE_NUMBERS_RIGHT_MARGIN 1


/* 1 for syntax highlighting */
#define SYNTAX_HIGHLIGHT  1

/* Default text color with and without syntax highlighting (ncurses defined) */
#define SCOLOR_DEFAULT      COLOR_LIGHT_GRAY

/* Background text color for selection (BlueberryOS specific ncurses color) */
#define SCOLOR_VISUAL       COLOR_DARK_GRAY

/* Color for search matches */
#define SCOLOR_MATCH        COLOR_LIGHT_BLUE

/* Line numbers color */
#define SCOLOR_LINE_NUMBERS COLOR_WHITE

/* Color pairs */
#define SPAIR_TOPBAR        1
#define SPAIR_CMDBAR        2
#define SPAIR_MATCH         3
#define SPAIR_LINE_NUMBERS  4
#define SPAIR_DEFAULT       5

/* Colors that syntax highlight uses (BlueberryOS specific ncurses colors) */
#define SCOLOR_NUMBERS      COLOR_LIGHT_BROWN
#define SCOLOR_STRINGS      COLOR_LIGHT_GREEN
//#define SCOLOR_COMMENTS   COLOR_DARK_GRAY
#define SCOLOR_COMMENTS     COLOR_LIGHT_CYAN
#define SCOLOR_SEPERATORS   COLOR_LIGHT_RED
#define SCOLOR_KEYWORDS     COLOR_LIGHT_MAGENTA
#define SCOLOR_TYPES        COLOR_WHITE
#define SCOLOR_SPECIALS     COLOR_LIGHT_MAGENTA
#define SCOLOR_INCLUDES     COLOR_LIGHT_RED

/* Syntax highlighting color pairs */
#define SPAIR_NUMBERS       7
#define SPAIR_STRINGS       9
#define SPAIR_COMMENTS      11
#define SPAIR_SEPERATORS    13
#define SPAIR_KEYWORDS      15
#define SPAIR_TYPES         17
#define SPAIR_SPECIALS      19
#define SPAIR_INCLUDES      21


/* Filetype syntax highlighting config */
#define HIGHLIGHT_NUMBERS    (1 << 0)
#define HIGHLIGHT_STRINGS    (1 << 1)
#define HIGHLIGHT_COMMENTS   (1 << 2)
#define HIGHLIGHT_SEPERATORS (1 << 3)
#define HIGHLIGHT_KEYWORDS   (1 << 4)
#define HIGHLIGHT_TYPES      (1 << 5)
#define HIGHLIGHT_SPECIALS   (1 << 6)
#define HIGHLIGHT_C_INCLUDE  (1 << 7)




typedef unsigned int sh_t;
typedef struct {
    char* name;
    char** filenames;
    char** filetypes;
    char** keywords;
    char** types;
    char* scomment;
    sh_t sh;
} highlight_table_t;

highlight_table_t highlightTable[] = {
    {
        "C",
        (char*[]) {NULL},
        (char*[]) {"c", "cpp", "h", NULL},
        (char*[]) {
            "auto", "break", "case", "continue", "default", "do", "else", "enum",
            "for", "goto", "if", "return", "sizeof", "offsetof", "struct", "switch",
            "typedef", "union", "while", NULL
        },
        (char*[]) {
            "char", "const", "double", "float", "int", "long", "register", "restrict", "short",
            "inline", "extern", "signed", "static", "unsigned", "void", "volatile", NULL
        },
        "//",
        HIGHLIGHT_NUMBERS | HIGHLIGHT_STRINGS | HIGHLIGHT_COMMENTS | HIGHLIGHT_SEPERATORS |
        HIGHLIGHT_KEYWORDS | HIGHLIGHT_TYPES | HIGHLIGHT_SPECIALS | HIGHLIGHT_C_INCLUDE
    },
    {
        "Python",
        (char*[]) {NULL},
        (char*[]) {"py", NULL},
        (char*[]) {
            "False", "await", "else", "import", "pass", "None", "break", "except",
            "in", "raise", "True", "class", "finally", "is", "return", "and",
            "continue", "for", "lambda", "try", "as", "def", "from", "nonlocal",
            "while", "assert", "del", "global", "not", "with", "async", "elif",
            "if", "or", "yield", NULL
        },
        (char*[]) {NULL},
        "#",
        HIGHLIGHT_NUMBERS | HIGHLIGHT_STRINGS | HIGHLIGHT_COMMENTS | HIGHLIGHT_SEPERATORS |
        HIGHLIGHT_KEYWORDS
    },
};

#define TAB_SIZE   (sizeof(TAB_RENDER)-1)
#define MAX_CMD_BUFFER 32
#define MAX_SEARCH_BUFFER 64

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

typedef enum {
    NORMAL,
    COMMAND,
    INSERT,
    VISUAL,
    VISUAL_LINE,
    SEARCH
} mode_t;

WINDOW* topBar;
WINDOW* cmdBar;


typedef void (*handler_t)(void);
typedef struct {
    int key;
    handler_t handler;
} mapping_t;

typedef struct {
    unsigned int len;
    unsigned int rlen;

#if SYNTAX_HIGHLIGHT
    unsigned char* colors;
#endif

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

    unsigned int omaxcols;
    unsigned int omaxrows;
    unsigned int maxcols;
    unsigned int maxrows;

#if LINE_NUMBERS
    unsigned int tdigits;
#endif

    unsigned int searchSize;
    unsigned int searchx;
    unsigned int searchy;

    highlight_table_t* ht;
#if SYNTAX_HIGHLIGHT
    highlight_table_t* pht;
#endif

    unsigned int clipSize;
    char* clipboard;
    char* filename;

    bool searchFound;
    bool searchPrev;
    bool saved;

    char searchBuffer[MAX_SEARCH_BUFFER+1];
} E;


extern inline row_t* currentRow(void) {
    return &E.rows[E.cury];
}

extern inline unsigned int getDigits(unsigned int n) {

    unsigned int r = 1;
    while (n > 9) {
        n /= 10;
        r++;
    }

    return r;
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

extern inline unsigned int curxToRCurx(row_t* row, unsigned int curx) {

    unsigned int rcurx = 0;
    for (unsigned int i = 0; i < curx; i++) {
        char c = row->chars[i];
        if (c == '\t')
            rcurx += TAB_SIZE - (rcurx % TAB_SIZE);
        else if (c == '\e')
            rcurx += 2;
        else
            rcurx++;
    }

    return rcurx;
}

/*
extern inline unsigned int rcurxToCurx(row_t* row, unsigned int curx, unsigned int rcurx) {

    unsigned int rx = 0;
    for (int i = 0; i < (int) curx - 1; i++) {
        char c = row->chars[i];
        if (c == '\t')
            rx += TAB_SIZE - (rx % TAB_SIZE);
        else if (c == '\e')
            rx += 2;
        else
            rx++;
    }

    if (rx < rcurx)
        return curx;

    return curx+1;
}
*/

inline bool is_rseperator(int c) {
    return strchr(",.()+-/*=~%<>[];", c) != NULL;
}

inline bool is_seperator(int c) {
    return c == '\0' || isspace(c) || is_rseperator(c);
}

#if SYNTAX_HIGHLIGHT
void updateSyntax(row_t* row) {

    free(row->colors);
    row->colors = (unsigned char*) malloc(row->rlen);
    memset(row->colors, SPAIR_DEFAULT, row->rlen);

    if (E.ht == NULL)
        return;

    char strchar = 0;
    bool string = false;
    bool comment = false;
    bool psep = true;
    unsigned char pcolor = 0;
    unsigned int i = 0;
    while (i < row->rlen) {
        char c = row->rchars[i];

        if (i > 0)
            pcolor = row->colors[i-1];

        if (string && c == strchar)
            string = false;
        else if (!string && (c == '"' || c == '\'')) {
            string = true;
            strchar = c;
        }

        bool sep = is_seperator(c);
        if ((E.ht->sh & HIGHLIGHT_COMMENTS) && (comment || (strncmp(&row->rchars[i], E.ht->scomment, strlen(E.ht->scomment)) == 0))) {
            row->colors[i++] = SPAIR_COMMENTS;
            comment = true;
            goto end;
        }

        if ((E.ht->sh & HIGHLIGHT_KEYWORDS) && psep) {
            for (unsigned int j = 0; E.ht->keywords[j]; j++) {

                unsigned int len = strlen(E.ht->keywords[j]);
                if (strncmp(&row->rchars[i], E.ht->keywords[j], len) == 0 && is_seperator(row->rchars[i + len])) {
                    memset(&row->colors[i], SPAIR_KEYWORDS, len);
                    i += len;
                    goto end;
                }
            }
        }

        if ((E.ht->sh & HIGHLIGHT_TYPES) && psep) {
            for (unsigned int j = 0; E.ht->types[j]; j++) {

                unsigned int len = strlen(E.ht->types[j]);
                if (strncmp(&row->rchars[i], E.ht->types[j], len) == 0) {
                    memset(&row->colors[i], SPAIR_TYPES, len);
                    i += len;
                    goto end;
                }
            }
        }

        if ((E.ht->sh & HIGHLIGHT_NUMBERS) && ((isdigit(c) && (psep || pcolor == SPAIR_NUMBERS)) || (c == '-' && isdigit(row->rchars[i+1])) || (c == '.' && pcolor == SPAIR_NUMBERS)))
            row->colors[i++] = SPAIR_NUMBERS;

        else if ((E.ht->sh & HIGHLIGHT_C_INCLUDE) && (c == '<')) {
            while (i < row->rlen && row->rchars[i] != '>') {
                row->colors[i++] = SPAIR_SEPERATORS;
            }
        }

        else if ((E.ht->sh & HIGHLIGHT_SEPERATORS) && is_rseperator(c))
            row->colors[i++] = SPAIR_SEPERATORS;

        else if ((E.ht->sh & HIGHLIGHT_STRINGS) && (string || c == '"' || c == '\''))
            row->colors[i++] = SPAIR_STRINGS;

        else if ((E.ht->sh & HIGHLIGHT_SPECIALS) && (c == '#')) {
            while (i < row->rlen && !(sep = is_seperator(row->rchars[i]))) {
                row->colors[i++] = SPAIR_SPECIALS;
            }
        }
        else
            i++;

end:
        psep = sep;
    }
}

void updateAllSyntax(void) {

    for (unsigned int i = 0; i < E.numrows; i++) {
        updateSyntax(&E.rows[i]);
    }
}
#endif

bool getSyntaxName(char* name) {

    for (unsigned int i = 0; i < (sizeof(highlightTable)/sizeof(highlightTable[0])); i++) {

        if (strcmp(highlightTable[i].name, name) == 0) {

            E.ht = &highlightTable[i];

#if SYNTAX_HIGHLIGHT
            if (E.ht != E.pht)
                updateAllSyntax();

            E.pht = E.ht;
#endif
            return true;
        }
    }

    return false;
}
bool getSyntaxFiletype(char* filetype) {

    for (unsigned int i = 0; i < (sizeof(highlightTable)/sizeof(highlightTable[0])); i++) {

        for (unsigned int j = 0; highlightTable[i].filetypes[j]; j++) {

            if (strcmp(highlightTable[i].filetypes[j], filetype) == 0) {

                E.ht = &highlightTable[i];

#if SYNTAX_HIGHLIGHT
                if (E.ht != E.pht)
                    updateAllSyntax();

                E.pht = E.ht;
#endif
                return true;
            }
        }
    }

    return false;
}

bool getSyntaxFilename(char* filename) {

    for (unsigned int i = 0; i < (sizeof(highlightTable)/sizeof(highlightTable[0])); i++) {

        for (unsigned int j = 0; highlightTable[i].filenames[j]; j++) {

            if (strcmp(highlightTable[i].filenames[j], filename) == 0) {

                E.ht = &highlightTable[i];

#if SYNTAX_HIGHLIGHT
                if (E.ht != E.pht)
                    updateAllSyntax();

                E.pht = E.ht;
#endif
                return true;
            }
        }
    }

    return false;
}

void getSyntax(char* filename) {

    E.ht = NULL;
    if (filename == NULL) {
#if SYNTAX_HIGHLIGHT
        if (E.ht != E.pht)
            updateAllSyntax();

        E.pht = E.ht;
#endif
        return;
    }

    if (getSyntaxFilename(filename))
        return;

    char* filetype = strrchr(filename, '.');
    if (filetype == NULL) {
#if SYNTAX_HIGHLIGHT
        if (E.ht != E.pht)
            updateAllSyntax();

        E.pht = E.ht;
#endif
        return;
    }
    if (getSyntaxFiletype(filetype + 1))
        return;

#if SYNTAX_HIGHLIGHT
    if (E.ht != E.pht)
        updateAllSyntax();

    E.pht = E.ht;
#endif
}

void updateMaxWidth(void) {

#if LINE_NUMBERS
    unsigned int d = getDigits(E.numrows);
    if (E.tdigits == d)
        return;

    E.tdigits = d;
    E.maxcols = E.omaxcols - d - LEFT_MARGIN - LINE_NUMBERS_RIGHT_MARGIN;
#else
    E.maxcols = E.omaxcols - LEFT_MARGIN;
#endif
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

            row->rchars = realloc(row->rchars, row->rlen + 2);
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

#if SYNTAX_HIGHLIGHT
    updateSyntax(row);
#endif
}

void searchFor(bool final) {

    E.searchPrev = false;

#if SYNTAX_HIGHLIGHT
    updateSyntax(&E.rows[E.searchy]);
#endif

    char* pos;
    bool lapped = false;
    unsigned int i = E.cury;
    while (i < E.numrows) {

        row_t* row = &E.rows[i];
        if ((pos = strstr(row->chars, E.searchBuffer)) != NULL) {

            E.curx = pos - row->chars;
            E.rcurx = curxToRCurx(row, E.curx);
            E.scurx = E.curx;
            E.rscurx = E.rcurx;

            E.searchFound = true;
            E.searchx = E.curx + 1;
            E.searchy = i;
            E.cury = i;

            if (E.rcurx >= E.coloff + E.maxcols - SEARCH_MARGIN_S - 1)
                E.coloff = E.rcurx - E.maxcols + SEARCH_MARGIN_S + 1;
            else if (E.rcurx < E.coloff)
                E.coloff = MAX((int) E.rcurx - SEARCH_MARGIN_S, 0);

            if (E.cury >= E.rowoff + E.maxrows - SEARCH_MARGIN_TB - 1)
                E.rowoff = E.cury - E.maxrows + SEARCH_MARGIN_TB + 1;
            else if (E.cury < E.rowoff)
                E.rowoff = MAX((int) E.cury - SEARCH_MARGIN_TB, 0);

#if SYNTAX_HIGHLIGHT
            memset(&E.rows[i].colors[E.rcurx], SPAIR_MATCH, strlen(E.searchBuffer));
#endif
            return;
        }

        i++;
        if ((i == E.numrows) && !lapped) {
            lapped = true;
            i = 0;
        }
    }

    if (!final)
        return;

    E.searchFound = false;
    mvwprintw(cmdBar, 0, 0, "Pattern not found: %s", E.searchBuffer);
    wclrtoeol(cmdBar);
}

void insertCharacter(row_t* row, unsigned int at, int ch) {

    row->chars = (char*) realloc(row->chars, row->len+2);
    memmove(&row->chars[at+1], &row->chars[at], row->len - at + 1);
    row->chars[at] = (char) ch;
    row->len++;
    renderRow(row);
}

void appendRow(char* s, unsigned int linelen) {

    // If E.maxrows is NULL then realloc will call malloc for us
    E.rows = (row_t*) realloc(E.rows, sizeof(row_t) * (E.numrows + 1));

    row_t* row = &E.rows[E.numrows];
    row->len = linelen;
    row->chars = (char*) malloc(linelen + 1);
    memcpy(row->chars, s, linelen+1);

    row->rchars = NULL;

#if SYNTAX_HIGHLIGHT
    row->colors = NULL;
#endif
    E.numrows++;

    renderRow(row);
}

void readFile(char* filename) {

    free(E.filename);

    unsigned int fnlen = strlen(filename);
    E.filename = malloc(fnlen + 1);
    memcpy(E.filename, filename, fnlen + 1);

    getSyntax(filename);

    //int fd = open(filename, O_RDONLY | O_CREAT, 0664);
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        int backup = errno;

        // Don't exit because file didn't exist
        if (backup == ENOENT) {
            E.saved = false;
            return;
        }

        delwin(topBar);
        delwin(cmdBar);
        endwin();

        printf("open error: %s\n", strerror(backup));
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

        printf("Appending '%s' with len %d\n", line, linelen);
        appendRow(line, linelen);
    }
    for(;;){}

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
    else if (st == SEARCH)       return "-- SEARCH --";
    else                         return "-- UNKNOWN --";
}

void updateTopBar(void) {

    mvwprintw(topBar, 0, 0, "%s\t%s%s\t%s\t%d/%d",
              stateToString(E.mode),
              E.filename ? E.filename : "[Empty]",
              E.saved ? "" : " (*)",
              E.ht ? E.ht->name : "Plain text",
              E.cury+1,
              E.numrows);

    wclrtoeol(topBar);
}

int saveFile(char* filename) {

    int fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0664);
    if (fd == -1) {
        int backup = errno;

        if (backup == EACCES) {
            mvwprintw(cmdBar, 0, 0, "Permission denied");
            wclrtoeol(cmdBar);
            return -1;
        }

        mvwprintw(cmdBar, 0, 0, "Failed to open file for writing");
        wclrtoeol(cmdBar);
        return -1;

        /*
        delwin(topBar);
        delwin(cmdBar);
        endwin();

        printf("open error: %s\n", strerror(backup));
        exit(1);
        */
    }

    getSyntax(filename);
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

        int bytesWritten = saveFile(E.filename);
        if (bytesWritten >= 0) {
            mvwprintw(cmdBar, 0, 0, "\"%s\" %dL, %dB written", E.filename, E.numrows, bytesWritten);
            wclrtoeol(cmdBar);
        }
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

        int bytesWritten = saveFile(filename);
        if (bytesWritten >= 0) {
            mvwprintw(cmdBar, 0, 0, "\"%s\" %dL, %dB written", filename, E.numrows, bytesWritten);
            wclrtoeol(cmdBar);
        }
    }
    else if (strcmp(buf, "wq") == 0) {
        if (!E.filename) {
            mvwprintw(cmdBar, 0, 0, "No filename");
            wclrtoeol(cmdBar);
            return;
        }

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

        int bytesWritten = saveFile(filename);
        if (bytesWritten == -1)
            return;

        delwin(topBar);
        delwin(cmdBar);
        endwin();
        exit(0);
    }
    else if (strcmp(buf, "syntax") == 0) {

#if SYNTAX_HIGHLIGHT
        getSyntax(E.filename);
#else
        mvwprintw(cmdBar, 0, 0, "Syntax highlighting is off");
        wclrtoeol(cmdBar);
#endif

    }
    else if (strncmp(buf, "syntax ", 7) == 0) {

#if SYNTAX_HIGHLIGHT
        char* language = buf + 7;
        if (strlen(language) == 0) {
            mvwprintw(cmdBar, 0, 0, "No filetype");
            wclrtoeol(cmdBar);
            return;
        }

        if (!(getSyntaxFiletype(language) || getSyntaxName(language))) {
            mvwprintw(cmdBar, 0, 0, "Invalid language");
            wclrtoeol(cmdBar);
        }
#else
        mvwprintw(cmdBar, 0, 0, "Syntax highlighting is off");
        wclrtoeol(cmdBar);
#endif
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

    unsigned int startx;
    unsigned int endx;
    if (E.vcury > E.cury) {
        startx = E.curx;
        endx = E.vcurx;
    } else {
        startx = E.vcurx;
        endx = E.curx;
    }

#if !LINE_NUMBERS && (LEFT_MARGIN > 0)
    char leftmargin[LEFT_MARGIN+1];
    memset(leftmargin, ' ', LEFT_MARGIN);
    leftmargin[LEFT_MARGIN] = '\0';
#endif

    size_t i;
    for (i = 0; (i < E.maxrows) && (i < E.numrows-E.rowoff); i++) {

        attron(COLOR_PAIR(SPAIR_LINE_NUMBERS));
#if LINE_NUMBERS
        unsigned int r = getDigits(i + E.rowoff + 1);
        for (unsigned int j = 0; j < E.tdigits + LEFT_MARGIN - r; j++)
            addch(' ');
        printw("%d ", i + E.rowoff + 1);
#else
    #if LEFT_MARGIN > 0

        printw("%s", leftmargin);
    #endif

#endif
        attroff(COLOR_PAIR(SPAIR_LINE_NUMBERS));

        unsigned int len = E.rows[i + E.rowoff].rlen;
        if (len <= E.coloff) {
            clrtoeol();
            printw("\n");
            continue;
        }

        len -= E.coloff;
        len = len > E.maxcols ? E.maxcols : len;

#if SYNTAX_HIGHLIGHT
        for (unsigned int j = 0; j < len; j++) {

            unsigned int absi = i + E.rowoff;
            unsigned int absj = j + E.coloff;

            char c = E.rows[absi].rchars[absj];
            unsigned char color = E.rows[absi].colors[absj];

            // Get the selected version of that color pair
            if (((E.mode == VISUAL) && (
                    ((absi == starty) && (absi == endy) && (startx <= absj) && (absj < endx))
                    ||
                    ((absi == starty) && (absi != endy) && (absj >= startx))
                    ||
                    ((starty < absi) && (absi < endy))
                    ||
                    ((absi == endy) && (absi != starty) && (absj < endx))
                    ))
                ||
                    ((E.mode == VISUAL_LINE) && (starty <= i + E.rowoff) && (i + E.rowoff <= endy)))
                color++;

            attron(COLOR_PAIR(color));
            addch(c);
            attroff(COLOR_PAIR(color));
        }
#else
        char* buf = malloc(len+1);
        if ((E.mode == VISUAL) && (starty - E.rowoff <= i) && (i <= endy - E.rowoff)) {

            // If the selection is on a single line
            if ((i == starty - E.rowoff) && (i == endy - E.rowoff)) {

                attron(COLOR_PAIR(SPAIR_DEFAULT));
                if (startx > 0) {
                    memcpy(buf, E.rows[i + E.rowoff].rchars + E.coloff, startx);
                    buf[startx] = '\0';
                    printw("%s", buf);
                }
                attroff(COLOR_PAIR(SPAIR_DEFAULT));

                attron(COLOR_PAIR(SPAIR_DEFAULT+1));
                memcpy(buf, E.rows[i + E.rowoff].rchars + E.coloff + startx, endx - startx);
                buf[endx - startx] = '\0';
                printw("%s", buf);
                attroff(COLOR_PAIR(SPAIR_DEFAULT+1));

                if (len + 1 > endx) {
                    attron(COLOR_PAIR(SPAIR_DEFAULT));
                    memcpy(buf, E.rows[i + E.rowoff].rchars + E.coloff + endx, len-endx);
                    buf[len-endx] = '\0';
                    printw("%s", buf);
                    attroff(COLOR_PAIR(SPAIR_DEFAULT));
                }
            }
            // If this row is where the cursor is
            else if (i == E.cury - E.rowoff) {

                short c1 = E.cury <= E.vcury ? SPAIR_DEFAULT   : SPAIR_DEFAULT+1;
                short c2 = E.cury <= E.vcury ? SPAIR_DEFAULT+1 : SPAIR_DEFAULT;
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
                attron(COLOR_PAIR(SPAIR_DEFAULT));
                if (E.vcurx > 0) {
                    memcpy(buf, E.rows[i + E.rowoff].rchars + E.coloff, E.vcurx);
                    buf[E.vcurx] = '\0';
                    printw("%s", buf);
                }
                attroff(COLOR_PAIR(SPAIR_DEFAULT));

                attron(COLOR_PAIR(SPAIR_DEFAULT+1));
                memcpy(buf, E.rows[i + E.rowoff].rchars + E.coloff + E.vcurx, len-E.vcurx);
                buf[len-E.vcurx] = '\0';
                printw("%s", buf);
                attroff(COLOR_PAIR(SPAIR_DEFAULT+1));
            }
            // If this row is in the middle of the selection
            else {
                attron(COLOR_PAIR(SPAIR_DEFAULT+1));
                memcpy(buf, E.rows[i + E.rowoff].rchars + E.coloff, len);
                buf[len] = '\0';
                printw("%s", buf);

                attroff(COLOR_PAIR(SPAIR_DEFAULT+1));
            }
        }
        else if ((E.mode == VISUAL_LINE) && (starty - E.rowoff <= i) && (i <= endy - E.rowoff)) {

            attron(COLOR_PAIR(SPAIR_DEFAULT+1));
            memcpy(buf, E.rows[i + E.rowoff].rchars + E.coloff, len);
            buf[len] = '\0';
            printw("%s", buf);

            attroff(COLOR_PAIR(SPAIR_DEFAULT+1));
        }
        else {

            attron(COLOR_PAIR(SPAIR_DEFAULT));
            memcpy(buf, E.rows[i + E.rowoff].rchars + E.coloff, len);
            buf[len] = '\0';
            printw("%s", buf);
            attroff(COLOR_PAIR(SPAIR_DEFAULT));
        }

        free(buf);
#endif

        clrtoeol();
        printw("\n");
    }

    // Draw '~' for empty lines
    attron(COLOR_PAIR(SPAIR_DEFAULT));
    for (; i < E.maxrows; i++) {

        printw("~");
        clrtoeol();
        if (i != E.maxrows-1)
            printw("\n");
    }
    attroff(COLOR_PAIR(SPAIR_DEFAULT));
}

void scrollUp(void) {

    if (E.rowoff == 0)
        return;

    E.rowoff--;
}

void scrollDown(void) {

    if (E.rowoff + E.maxrows == E.numrows)
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
#if LINE_NUMBERS
    move(E.cury - E.rowoff, E.rcurx - E.coloff + E.tdigits + LEFT_MARGIN + 1);
#else
    move(E.cury - E.rowoff, E.rcurx - E.coloff + LEFT_MARGIN);
#endif
}



void leftArrow(void) {

    if ((E.rcurx - E.coloff == 0) && (E.cury - E.rowoff == 0))
        return;

    if (E.rcurx - E.coloff <= SCROLL_MARGIN_S) {

        if (E.coloff > 0) {
            scrollLeft();

            E.curx--;
#if TAB_AS_SPACE
            E.rcurx--;
            //E.curx = rcurxToCurx(currentRow(), E.curx, E.rcurx);
#else
            E.rcurx = curxToRCurx(currentRow(), E.curx);
#endif
        } else if (E.rcurx == 0) {
            if (E.cury - E.rowoff <= SCROLL_MARGIN_TB)
                scrollUp();
            else
                E.cury--;

            E.rcurx = currentRow()->rlen;
            E.curx = currentRow()->len;
            if (E.rcurx > E.maxcols) {
                E.coloff = E.rcurx - E.maxcols + E.maxcols/2;
            }
        } else {

            E.curx--;
#if TAB_AS_SPACE
            E.rcurx--;
            //E.curx = rcurxToCurx(currentRow(), E.curx, E.rcurx);
#else
            E.rcurx = curxToRCurx(currentRow(), E.curx);
#endif
        }
    } else {

        E.curx--;
#if TAB_AS_SPACE
        E.rcurx--;
        //E.curx = rcurxToCurx(currentRow(), E.curx, E.rcurx);
#else
        E.rcurx = curxToRCurx(currentRow(), E.curx);
#endif

    }

    E.rscurx = E.rcurx;
    E.scurx = E.curx;
}

void rightArrow(void) {

    if ((E.numrows == 0) || ((E.cury >= E.numrows-1)))
        return;

    if (E.rcurx >= currentRow()->rlen) {

        if (E.cury - E.rowoff >= E.maxrows - SCROLL_MARGIN_TB - 1)
            scrollDown();

        E.cury++;
        E.rcurx = 0;
        E.curx = 0;

        if (E.coloff > 0) {
            E.coloff = 0;
        }

    } else if (E.rcurx - E.coloff >= E.maxcols - SCROLL_MARGIN_S - 1) {

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

    if (E.cury - E.rowoff <= SCROLL_MARGIN_TB)
        scrollUp();

    E.cury--;

    snapCursor();

    if (E.rcurx < E.maxcols-1)
        E.coloff = 0;
    else
        E.coloff = (E.rcurx+E.maxcols > E.coloff || E.rcurx < E.coloff) ? (E.rcurx-E.maxcols+E.maxcols/2) : (E.coloff);
}

void downArrow(void) {

    if ((E.numrows == 0) || (E.cury == E.numrows-1))
        return;

    if (E.cury - E.rowoff >= E.maxrows - SCROLL_MARGIN_TB - 1)
        scrollDown();

    E.cury++;

    snapCursor();

    if (E.rcurx < E.maxcols-1)
        E.coloff = 0;
    else
        E.coloff = (E.rcurx+E.maxcols > E.coloff || E.rcurx < E.coloff) ? (E.rcurx-E.maxcols+E.maxcols/2) : (E.coloff);
}

void escapeKey(void) {

    E.mode = NORMAL;
    werase(cmdBar);

#if SYNTAX_HIGHLIGHT
    updateSyntax(&E.rows[E.searchy]);
#endif
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

    E.coloff = E.rscurx > E.maxcols ? E.rscurx-E.maxcols+E.maxcols/2 : 0;
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
    E.rowoff = E.numrows-E.maxrows;
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
#if SYNTAX_HIGHLIGHT
    trow->colors = NULL;
#endif

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

    if (E.cury - E.rowoff >= E.maxrows)
        E.rowoff++;
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
        if (E.rcurx > E.maxcols) {
            E.coloff = E.rcurx - E.maxcols + E.maxcols/2;
        }

        trow->chars = (char*) realloc(trow->chars, trow->len + frow->len + 1);
        memcpy(&trow->chars[trow->len], frow->chars, frow->len+1);
        trow->len += frow->len;

        renderRow(trow);

        if (E.cury < E.numrows-1)
            memmove(&E.rows[E.cury], &E.rows[E.cury+1], sizeof(row_t) * (E.numrows - E.cury - 1));
        E.rows = (row_t*) realloc(E.rows, sizeof(row_t) * (E.numrows-1));
        E.numrows--;

        if ((E.rowoff > 0) && (E.cury - E.rowoff < SCROLL_MARGIN_TB))
            E.rowoff--;

        E.cury--;

        E.saved = false;
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

    /*
    if (row->chars[E.curx] == '\t')
        E.rcurx = curxToRCurx(E.curx);
    else if (row->chars[E.curx] == '\e')
        E.rcurx -= 2;
    else
        E.rcurx--;
    */

    E.rcurx = curxToRCurx(currentRow(), E.curx);

    E.rscurx = E.rcurx;
    E.scurx = E.curx;

    E.saved = false;
}

void deleteCurrentLine(void) {

    if (E.numrows == 0)
        return;

    if (E.cury < E.numrows-1)
        memmove(&E.rows[E.cury], &E.rows[E.cury+1], sizeof(row_t) * (E.numrows - E.cury - 1));

    E.rows = (row_t*) realloc(E.rows, sizeof(row_t) * (E.numrows-1));
    E.numrows--;

    snapCursor();

    E.saved = false;
}

void copySelection(void) {

    free(E.clipboard);
    E.clipboard = NULL;

    unsigned int starty = MIN(E.cury, E.vcury);
    unsigned int endy = MAX(E.cury, E.vcury);

    E.clipSize = 0;
    if (E.mode == VISUAL) {

        // If the selection is on the same line
        if (starty == endy) {

            unsigned int startx = MIN(E.curx, E.vcurx);
            unsigned int endx = MAX(E.curx, E.vcurx);

            unsigned int len = endx - startx;
            if (len == 0) {
                E.mode = NORMAL;
                return;
            }

            E.clipboard = (char*) realloc(E.clipboard, len + 1);
            memcpy(E.clipboard, &E.rows[starty].chars[startx], len);
            E.clipboard[len] = '\0';
            E.clipSize = len;
        }
        else {

            unsigned int startx;
            unsigned int endx;

            if (E.vcury > E.cury) {
                startx = E.curx;
                endx = E.vcurx;
            } else {
                startx = E.vcurx;
                endx = E.curx;
            }

            int startlen = E.rows[starty].len - startx;
            E.clipboard = (char*) malloc(startlen + 1);
            memcpy(E.clipboard, &E.rows[starty].chars[startx], startlen);
            E.clipboard[startlen] = '\n';
            E.clipSize = startlen + 1;

            for (unsigned int y = starty+1; y <= endy-1; y++) {

                E.clipboard = (char*) realloc(E.clipboard, E.clipSize + E.rows[y].len + 1);
                memcpy(&E.clipboard[E.clipSize], E.rows[y].chars, E.rows[y].len);
                E.clipboard[E.clipSize + E.rows[y].len] = '\n';
                E.clipSize += E.rows[y].len + 1;
            }

            E.clipboard = (char*) realloc(E.clipboard, E.clipSize + endx + 2);
            memcpy(&E.clipboard[E.clipSize], E.rows[endy].chars, endx);
            E.clipboard[E.clipSize + endx] = '\n';
            E.clipboard[E.clipSize + endx + 1] = '\0';
            E.clipSize += endx + 2;
        }
    }
    //else if (E.mode == VISUAL_LINE) {
    else {

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
    printf("%d\n'%s'", E.clipSize, E.clipboard);
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

            if (row->len > 0)
                memmove(&row->chars[E.curx + len], &row->chars[E.curx], row->len - E.curx);

            memcpy(&row->chars[E.curx], &E.clipboard[i], len);
            row->len += len;
            E.curx += len;
        }

        if (foundNewline) {

            E.curx = 0;

            row->chars = (char*) realloc(row->chars, row->len + 1);
            row->chars[row->len] = '\0';
            renderRow(row);

            E.cury++;
            if (E.cury - E.rowoff >= E.maxrows)
                E.rowoff++;

            E.rows = (row_t*) realloc(E.rows, sizeof(row_t) * (E.numrows + 1));
            memmove(&E.rows[E.cury+1], &E.rows[E.cury], sizeof(row_t) * (E.numrows - E.cury));
            E.numrows++;

            row_t* r = &E.rows[E.cury];
            r->chars = (char*) malloc(1);
            r->chars[0] = '\0';
            r->rchars = NULL;
#if SYNTAX_HIGHLIGHT
            r->colors = NULL;
#endif
            r->len = 0;
            r->rlen = 0;
            renderRow(r);
        }

        i = end;
    }

    if (row == NULL)
        return;

    row->chars = (char*) realloc(row->chars, row->len + 1);
    row->chars[row->len] = '\0';

    E.rcurx = curxToRCurx(row, E.curx);
    E.rscurx = E.rcurx;
    E.scurx = E.curx;

    if (E.rcurx > E.maxcols)
        E.coloff = E.rcurx - E.maxcols + E.maxcols/2;

    renderRow(row);

    E.saved = false;
}

void newLineAndInsert(void) {

    // TODO auto-indent
    E.rscurx = 0;
    E.scurx = 0;
    E.rcurx = 0;
    E.curx = 0;
    E.cury++;
    if (E.cury - E.rowoff >= E.maxrows)
        E.rowoff++;

    E.rows = (row_t*) realloc(E.rows, sizeof(row_t) * (E.numrows + 1));
    memmove(&E.rows[E.cury+1], &E.rows[E.cury], sizeof(row_t) * (E.numrows - E.cury));
    E.numrows++;

    row_t* r = &E.rows[E.cury];
    r->chars = malloc(1);
#if SYNTAX_HIGHLIGHT
    r->colors = NULL;
#endif
    r->chars[0] = '\0';
    r->rchars = NULL;
    r->len = 0;
    r->rlen = 0;
    renderRow(r);

    E.mode = INSERT;

    E.saved = false;
}

void searchNext(void) {

    if ((E.searchSize == 0) || !E.searchFound)
        return;

    unsigned int searchy;
    if (E.cury == E.searchy)
        searchy = E.searchy;
    else
        searchy = E.cury;

#if SYNTAX_HIGHLIGHT
    updateSyntax(&E.rows[E.searchy]);
#endif

    char* pos;
    bool lapped = false;
    unsigned int i = searchy;

    if (E.searchPrev) {
        E.searchPrev = false;
        pos = strstr(&E.rows[i].chars[E.searchx], E.searchBuffer);
        if (pos == NULL)
            i++;
        else
            E.searchx = pos - E.rows[i].chars + 1;
    }

    while (i < E.numrows) {

        row_t* row = &E.rows[i];
        if ((pos = strstr(&row->chars[E.searchx], E.searchBuffer)) != NULL) {

            E.curx = pos - row->chars;
            E.rcurx = curxToRCurx(row, E.curx);
            E.scurx = E.curx;
            E.rscurx = E.rcurx;

            E.searchx = E.curx + 1;
            E.searchy = i;
            E.cury = i;

            if (E.rcurx >= E.coloff + E.maxcols - SEARCH_MARGIN_S - 1)
                E.coloff = E.rcurx - E.maxcols + SEARCH_MARGIN_S + 1;
            else if (E.rcurx < E.coloff)
                E.coloff = MAX((int) E.rcurx - SEARCH_MARGIN_S, 0);

            if (E.cury >= E.rowoff + E.maxrows - SEARCH_MARGIN_TB - 1)
                E.rowoff = E.cury - E.maxrows + SEARCH_MARGIN_TB + 1;
            else if (E.cury < E.rowoff)
                E.rowoff = MAX((int) E.cury - SEARCH_MARGIN_TB, 0);

#if SYNTAX_HIGHLIGHT
            memset(&E.rows[i].colors[E.rcurx], SPAIR_MATCH, strlen(E.searchBuffer));
#endif
            return;
        }
        E.searchx = 0;

        i++;
        if ((i == E.numrows) && !lapped) {
            lapped = true;
            i = 0;
        }
    }
}

void searchPrevious(void) {

    if ((E.searchSize == 0) || !E.searchFound)
        return;

    E.searchPrev = true;

    unsigned int searchy;
    if (E.cury == E.searchy)
        if (E.searchy == 0)
            searchy = E.numrows - 1;
        else
            searchy = E.searchy;
    else
        searchy = E.cury;

#if SYNTAX_HIGHLIGHT
    updateSyntax(&E.rows[E.searchy]);
#endif

    char* pos;
    bool lapped = false;
    int i = searchy;
    while (i >= 0) {

        row_t* row = &E.rows[i];
        char* buf = malloc(E.searchx + 1);
        memcpy(buf, row->chars, E.searchx);
        buf[E.searchx] = '\0';
        if ((pos = strrstr(buf, E.searchBuffer)) != NULL) {
            E.curx = pos - buf;
            E.rcurx = curxToRCurx(row, E.curx);
            E.scurx = E.curx;
            E.rscurx = E.rcurx;

            if (E.curx == 0) {
                if (i == 0)
                    E.searchy = E.numrows-1;
                else
                    E.searchy = i - 1;
                E.searchx = E.rows[i].len;
            }
            else {
                E.searchx = E.curx - 1;
                E.searchy = i;
            }
            E.cury = i;

            if (E.rcurx >= E.coloff + E.maxcols - SEARCH_MARGIN_S - 1)
                E.coloff = E.rcurx - E.maxcols + SEARCH_MARGIN_S + 1;
            else if (E.rcurx < E.coloff)
                E.coloff = MAX((int) E.rcurx - SEARCH_MARGIN_S, 0);

            if (E.cury >= E.rowoff + E.maxrows - SEARCH_MARGIN_TB - 1)
                E.rowoff = E.cury - E.maxrows + SEARCH_MARGIN_TB + 1;
            else if (E.cury < E.rowoff)
                E.rowoff = MAX((int) E.cury - SEARCH_MARGIN_TB, 0);

#if SYNTAX_HIGHLIGHT
            memset(&E.rows[i].colors[E.rcurx], SPAIR_MATCH, strlen(E.searchBuffer));
#endif
            return;
        }
        free(buf);

        if (i == 0) {
            if (lapped)
                return;

            lapped = true;
            i = E.numrows;
        }

        E.searchx = E.rows[i-1].len;
        i--;
    }
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
    {'o', newLineAndInsert},
    {'n', searchNext},
    {'m', searchPrevious},
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
    {'y', copySelection},
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

    E.mode = NORMAL;

    E.rscurx = 0;
    E.rcurx = 0;
    E.scurx = 0;
    E.curx = 0;
    E.cury = 0;

    E.searchFound = false;
    E.searchPrev = false;
    E.searchx = 0;
    E.searchy = 0;

#if SYNTAX_HIGHLIGHT
    E.pht = NULL;
    E.ht = NULL;
#endif

    E.clipSize = 0;
    E.clipboard = NULL;
    E.filename = NULL;
    E.saved = false;


    /* Top-bar color */
    init_pair(SPAIR_TOPBAR, COLOR_BLACK, COLOR_WHITE);

    /* Cmd-bar color */
    init_pair(SPAIR_CMDBAR, COLOR_LIGHT_GRAY, COLOR_BLACK);

    /* Color for search matches */
    init_pair(SPAIR_MATCH,     SCOLOR_DEFAULT,  SCOLOR_MATCH);

    init_pair(SPAIR_LINE_NUMBERS, SCOLOR_LINE_NUMBERS, COLOR_BLACK);

    /* Default text color */
    init_pair(SPAIR_DEFAULT,   SCOLOR_DEFAULT,  COLOR_BLACK);
    init_pair(SPAIR_DEFAULT+1, SCOLOR_DEFAULT,  SCOLOR_VISUAL);

    /* Syntax highlighting colors */
#if SYNTAX_HIGHLIGHT
    init_pair(SPAIR_NUMBERS,      SCOLOR_NUMBERS,    COLOR_BLACK);
    init_pair(SPAIR_NUMBERS+1,    SCOLOR_NUMBERS,    SCOLOR_VISUAL);

    init_pair(SPAIR_STRINGS,      SCOLOR_STRINGS,    COLOR_BLACK);
    init_pair(SPAIR_STRINGS+1,    SCOLOR_STRINGS,    SCOLOR_VISUAL);

    init_pair(SPAIR_COMMENTS,     SCOLOR_COMMENTS,   COLOR_BLACK);
    init_pair(SPAIR_COMMENTS+1,   SCOLOR_COMMENTS,   SCOLOR_VISUAL);

    init_pair(SPAIR_SEPERATORS,   SCOLOR_SEPERATORS, COLOR_BLACK);
    init_pair(SPAIR_SEPERATORS+1, SCOLOR_SEPERATORS, SCOLOR_VISUAL);

    init_pair(SPAIR_KEYWORDS,     SCOLOR_KEYWORDS,   COLOR_BLACK);
    init_pair(SPAIR_KEYWORDS+1,   SCOLOR_KEYWORDS,   SCOLOR_VISUAL);

    init_pair(SPAIR_TYPES,        SCOLOR_TYPES,      COLOR_BLACK);
    init_pair(SPAIR_TYPES+1,      SCOLOR_TYPES,      SCOLOR_VISUAL);

    init_pair(SPAIR_SPECIALS,     SCOLOR_SPECIALS,   COLOR_BLACK);
    init_pair(SPAIR_SPECIALS+1,   SCOLOR_SPECIALS,   SCOLOR_VISUAL);

    init_pair(SPAIR_INCLUDES,     SCOLOR_INCLUDES,   COLOR_BLACK);
    init_pair(SPAIR_INCLUDES+1,   SCOLOR_INCLUDES,   SCOLOR_VISUAL);
#endif

    E.omaxcols = getmaxx(stdscr);
    E.omaxrows = getmaxy(stdscr);

    topBar = newwin(1, E.omaxcols, E.omaxrows-2, 0);
    cmdBar = newwin(1, E.omaxcols, E.omaxrows-1, 0);
    E.omaxrows -= 2;

    E.maxrows = E.omaxrows;
    E.maxcols = E.omaxcols;

    refresh();


    if (argc > 1)
        readFile(argv[1]);

    if (E.numrows == 0)
        appendRow("", 0);

    updateMaxWidth();

    char cmdBuffer[MAX_CMD_BUFFER+1];
    unsigned int cmdCursor = 0;
    unsigned int cmdSize = 0;

    unsigned int searchCursor = 0;
    int ch;

    //clear();
    //refresh();
    wattron(topBar, COLOR_PAIR(SPAIR_TOPBAR));
    wattron(cmdBar, COLOR_PAIR(SPAIR_CMDBAR));
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
            rightArrow();
            E.saved = false;
        }
        else if (E.mode == NORMAL) {

            if (ch == ':') {
                E.mode = COMMAND;
                cmdSize = 0;
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
            else if (ch == '/') {
                E.mode = SEARCH;
                E.searchSize = 0;
                searchCursor = 0;
                mvwprintw(cmdBar, 0, 0, "/");
                wclrtoeol(cmdBar);
            }
            else if (ch == 'A') {
                E.mode = INSERT;
                gotoEndOfLine();
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
        else if (E.mode == SEARCH) {

            if (searchCursor > MAX_SEARCH_BUFFER) {
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
                E.searchBuffer[E.searchSize] = '\0';
                searchFor(true);
            }
            else if (ch == KEY_LEFT) {
                if (searchCursor == 0)
                    continue;

                wmove(cmdBar, 0, --searchCursor+1);
            }
            else if (ch == KEY_RIGHT) {
                if (searchCursor == E.searchSize)
                    continue;

                wmove(cmdBar, 0, ++searchCursor+1);
            }
            else if (ch == '\b' || ch == KEY_BACKSPACE) {
                if (searchCursor == 0)
                    continue;

                mvwprintw(cmdBar, 0, searchCursor, " ");
                E.searchBuffer[searchCursor] = ' ';
                wmove(cmdBar, 0, searchCursor--);
                E.searchSize--;
            }
            else if (ch == 127) {
                wdelch(cmdBar);
                E.searchSize--;
            }
            else {

                if (searchCursor == E.searchSize) {
                    E.searchBuffer[searchCursor++] = ch;
                    E.searchSize++;
                    wprintw(cmdBar, "%c", ch);
                    wclrtoeol(cmdBar);
                }
                else {
                    memmove(&E.searchBuffer[searchCursor+1], &E.searchBuffer[searchCursor], E.searchSize-searchCursor);
                    E.searchBuffer[searchCursor++] = ch;
                    E.searchBuffer[++E.searchSize] = '\0';
                    mvwprintw(cmdBar, 0, 0, "/%s", E.searchBuffer);
                    wclrtoeol(cmdBar);
                    wmove(cmdBar, 0, searchCursor+1);
                }

                E.searchBuffer[E.searchSize] = '\0';
                searchFor(false);
            }
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

