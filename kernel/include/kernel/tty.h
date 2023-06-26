
#ifndef KERNEL_TTY_H
#define KERNEL_TTY_H

#include <stddef.h>
#include <stdbool.h>

/* Color constants */
#define VGA_BLACK 0
#define VGA_BLUE 1
#define VGA_GREEN 2
#define VGA_CYAN 3
#define VGA_RED 4
#define VGA_MAGENTA 5
#define VGA_BROWN 6
#define VGA_LIGHT_GRAY 7
#define VGA_DARK_GRAY 8
#define VGA_LIGHT_BLUE 9
#define VGA_LIGHT_GREEN 10
#define VGA_LIGHT_CYAN 11
#define VGA_LIGHT_RED 12
#define VGA_LIGHT_MAGENTA 13
#define VGA_LIGHT_BROWN 14
#define VGA_WHITE 15

void terminal_initialize(size_t width, size_t height, void* buffer);
void terminal_move_cursor(size_t x, size_t y);
void terminal_write(const char* data, size_t size);
void terminal_writechar(const char c, bool updateCursor);
void terminal_writestring(const char* data);

#endif /* KERNEL_TTY_H */

