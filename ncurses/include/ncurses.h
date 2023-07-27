
#ifndef _NCURSES_H
#define _NCURSES_H 1

#include <sys/cdefs.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>


#define OK   0
#define ERR -1

typedef struct {
    short fg;
    short bg;
} color_pair_t;

typedef struct {

    unsigned int curx;
    unsigned int cury;

    unsigned int width;
    unsigned int height;

    unsigned int startx;
    unsigned int starty;

    bool toclear;

    char* buf;
    char* lineschanged;
    char* colors;

    unsigned char curColor;

} WINDOW;

extern WINDOW* stdscr;



#define KEY_DOWN        (25+256)        /* down-arrow key */
#define KEY_UP          (24+256)        /* up-arrow key */
#define KEY_LEFT        (27+256)        /* left-arrow key */
#define KEY_RIGHT       (26+256)        /* right-arrow key */

#define KEY_HOME        0406            /* home key */
#define KEY_BACKSPACE   0407            /* backspace key */
#define KEY_F0          0410            /* Function keys.  Space for 64 */
#define KEY_F(n)        (KEY_F0+(n))    /* Value of function key n */



#define getmaxx(win)            ((win) ? ((win)->width) : ERR)
#define getmaxy(win)            ((win) ? ((win)->height) : ERR)
#define getmaxyx(win,y,x)       (y = getmaxy(win), x = getmaxx(win))


#define COLOR_BLACK         0
#define COLOR_BLUE          1
#define COLOR_GREEN         2
#define COLOR_CYAN          3
#define COLOR_RED           4
#define COLOR_MAGENTA       5
#define COLOR_BROWN         6
#define COLOR_WHITE         7
#define COLOR_LIGHT_GRAY    7
#define COLOR_DARK_GRAY     8
#define COLOR_LIGHT_BLUE    9
#define COLOR_LIGHT_GREEN   10
#define COLOR_LIGHT_CYAN    11
#define COLOR_LIGHT_RED     12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_LIGHT_BROWN   14
//#define COLOR_WHITE         15


#ifdef __cplusplus
extern "C" {
#endif

WINDOW* initscr(void);
WINDOW* newwin(unsigned int height, unsigned int width, unsigned int starty, unsigned int startx);
int endwin(void);
int delwin(WINDOW* win);

inline int noecho(void) { return OK; }

int init_pair(short pair, short f, short b);
inline int has_colors(void) { return 1; };
inline int start_color(void) { return OK; }

int wattron(WINDOW* win, int attrs);
inline int attron(int attrs) { return wattron(stdscr, attrs); }

int wattroff(WINDOW* win, int attrs);
inline int attroff(int attrs) { return wattroff(stdscr, attrs); }

#define COLOR_PAIR(n) ((int) (n & 0xFF))

int printw(char*, ...);
int wprintw(WINDOW* win, const char* format, ...);
int mvwprintw(WINDOW* win, int y, int x, const char* format, ...);
int vmvwprintw(WINDOW* win, int y, int x, const char*, va_list args);

bool wputcharw(WINDOW* win, char c);

int refresh(void);
int wrefresh(WINDOW* win);

int getch(void);
int move(int y, int x);
int wmove(WINDOW* win, int y, int x);

int clear(void);
int wclear(WINDOW* win);

int erase(void);
int werase(WINDOW* win);

int delch(void);
int wdelch(WINDOW* win);

int clrtoeol(void);
int wclrtoeol(WINDOW* win);

#ifdef __cplusplus
}
#endif

#endif

