
#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>


int getch(void) {
    int buf = 0;
    read(STDIN_FILENO, &buf, 1);

    // Check for special character
    if (buf == '\e') {
        buf = 0;
        read(STDIN_FILENO, &buf, 1);

        // Escape
        if (buf == '[')
            buf = '\e';
        else
            buf += 256;
    }

    return buf;
}

