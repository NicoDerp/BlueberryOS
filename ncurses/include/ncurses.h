
#ifndef _NCURSES_H
#define _NCURSES_H 1

#include <sys/cdefs.h>
#include <stdint.h>
#include <stdarg.h>


#define OK   0
#define ERR -1

typedef struct {

    uint32_t curx;
    uint32_t cury;

} WINDOW;

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

