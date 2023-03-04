
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/tty.h>

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;

uint8_t terminal_color;

inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t) color << 8 | (uint16_t) c;
}

inline uint8_t vga_color(uint8_t fg, uint8_t bg) {
    return bg << 4 | fg;
}

void terminal_initialize(void) {
    // Set our copy of the cursor to upper left
    terminal_row = 0;
    terminal_column = 0;
    
    uint16_t* terminal_buffer = (uint16_t*) 0xB8000; // Set buffer to start of framebuffer

    // Loop through every cell and clear it
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', vga_color(VGA_LIGHT_GRAY, VGA_BLACK));
        }
    }
}

void terminal_scroll_down(void) {
    uint16_t* terminal_buffer = (uint16_t*) 0xB8000; // Set buffer to start of framebuffer

    terminal_row = VGA_HEIGHT - 1;

    for (size_t y = 0; y < VGA_HEIGHT-1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index_cur = y * VGA_WIDTH + x;
            const size_t index_next = index_cur + VGA_WIDTH;
            terminal_buffer[index_cur] = terminal_buffer[index_next];
        }
    }
}

void terminal_writechar(const char c) {
    uint16_t* terminal_buffer = (uint16_t*) 0xB8000; // Set buffer to start of framebuffer

    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;

        if (terminal_row >= VGA_HEIGHT) {
            terminal_scroll_down();
        }
        return;
    }

    const size_t index = terminal_row * VGA_WIDTH + terminal_column;
    terminal_buffer[index] = vga_entry(c, vga_color(VGA_LIGHT_GRAY, VGA_BLACK));

    terminal_column++;
    if (terminal_column >= VGA_WIDTH) {
        terminal_column = 0;
        terminal_row++;

        if (terminal_row >= VGA_HEIGHT) {
            terminal_scroll_down();
        }
    }
}

void terminal_write(const char* string, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_writechar(string[i]);
    }
}

void terminal_writestring(const char* string) {
    size_t i = 0;
    while (string[i] != 0) {
        terminal_writechar(string[i]);
        i++;
    }
}

