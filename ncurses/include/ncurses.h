
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

    unsigned int curx;
    unsigned int cury;

    unsigned int width;
    unsigned int height;

    unsigned int startx;
    unsigned int starty;

    bool toclear;

    char* buf;
    char* lineschanged;

} WINDOW;




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

#define noecho()

extern WINDOW* stdscr;

#ifdef __cplusplus
extern "C" {
#endif

WINDOW* initscr(void);
WINDOW* newwin(unsigned int height, unsigned int width, unsigned int starty, unsigned int startx);
int endwin(void);
int delwin(WINDOW* win);

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

