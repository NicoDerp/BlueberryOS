
#ifndef _NCURSES_H
#define _NCURSES_H 1

#include <sys/cdefs.h>
#include <stdint.h>
#include <stdarg.h>


#define OK   0
#define ERR -1

typedef struct {

    unsigned int curx;
    unsigned int cury;

    unsigned int width;
    unsigned int height;

    unsigned int startx;
    unsigned int starty;

    char* buf;

} WINDOW;

#define getmaxx(win)            ((win) ? ((win)->width) : ERR)
#define getmaxy(win)            ((win) ? ((win)->height) : ERR)
#define getmaxyx(win,y,x)       (y = getmaxy(win), x = getmaxx(win))

extern WINDOW* stdscr;

#ifdef __cplusplus
extern "C" {
#endif

WINDOW* initscr(void);
WINDOW* newwin(unsigned int height, unsigned int width, unsigned int starty, unsigned int startx);
int endwin(void);

int printw(char*, ...);
int wprintw(WINDOW* win, const char* format, ...);
int mvwprintw(WINDOW* win, int y, int x, const char* format, ...);

void refresh(void);
int wrefresh(WINDOW* win);

int getch(void);
int move(int y, int x);

int clear(void);
int wclear(WINDOW* win);

#ifdef __cplusplus
}
#endif

#endif

