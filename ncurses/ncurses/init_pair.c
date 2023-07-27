
#include <ncurses.h>


color_pair_t color_pairs[255];

int init_pair(short pair, short f, short b) {

    unsigned char index = (unsigned char) (pair & 0xFF);
    color_pairs[index].fg = f;
    color_pairs[index].bg = b;

    return OK;
}

