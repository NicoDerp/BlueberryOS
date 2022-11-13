
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/tty.h>

#include "vga.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;

uint16_t* terminal_buffer;
uint8_t terminal_color;


void terminal_initialize(void) {
    terminal_buffer = (uint16_t*) 0xB8000; // Set buffer to start of framebuffer

    // Set our copy of the cursor to upper left
    terminal_row = 0;
    terminal_column = 0;

    // Set default color
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GRAY, VGA_COLOR_BLACK);
    
    // Loop through every cell and clear it
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_writechar(const char c) {
    const size_t index = terminal_row * VGA_WIDTH + terminal_column;
    terminal_buffer[index] = vga_entry(c, terminal_color);

    terminal_column++;
    if (terminal_column >= VGA_HEIGHT) {
        terminal_column = 0;
        terminal_row++;

        if (terminal_row >= VGA_WIDTH) {
            terminal_row = 0;
        }
    }
}

void terminal_write(const char* string, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_writechar(string[i]);
    }
}

void terminal_writestring(const char* string) {
    terminal_write(string, strlen(string));
}

