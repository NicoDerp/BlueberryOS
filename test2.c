
#include <stdio.h>
#include <ncurses.h>


void main(void) {

    int ch;
    WINDOW* my_window;
    WINDOW *my_scroller;

    initscr();
    raw();
    keypad(stdscr, TRUE);
    //noecho();
    
    if ((my_window = newwin(10, 20, 3, 4)) != 0) {
        box(my_window, 0, 0);
        wrefresh(my_window);
        if ((my_scroller = derwin(my_window, 8, 18, 1, 1)) != 0) {
            scrollok(my_scroller, TRUE);
            while ((ch=wgetch(my_scroller)) != 'q') {
                wprintw(my_scroller, "%#x - %s\n", ch, keyname(ch));
            }
        }
    }

    endwin();
}

