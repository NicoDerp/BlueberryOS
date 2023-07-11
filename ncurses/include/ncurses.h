
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

    unsigned int maxx;
    unsigned int maxy;

} WINDOW;

#define getmaxx(win)            ((win) ? ((win)->maxx) : ERR)
#define getmaxy(win)            ((win) ? ((win)->maxy) : ERR)
#define getmaxyx(win,y,x)       (y = getmaxy(win), x = getmaxx(win))

extern WINDOW* stdscr;

#ifdef __cplusplus
extern "C" {
#endif

WINDOW* initscr(void);
int printw(char*, ...);
void refresh(void);
int getch(void);
int endwin(void);

#ifdef __cplusplus
}
#endif

#endif

