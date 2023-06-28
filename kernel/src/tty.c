
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/tty.h>
#include <kernel/io.h>

#include <stdio.h>

static size_t VGA_WIDTH = 80;
static size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;

uint16_t* terminal_buffer;

uint8_t terminal_fg;
uint8_t terminal_bg;

char insideEscape = false;
uint32_t escapeIndex = 0;
bool setBackground = false;

inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t) color << 8 | (uint16_t) c;
}

inline uint8_t vga_color(uint8_t fg, uint8_t bg) {
    return bg << 4 | fg;
}

void enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {

    io_outb(0x3D4, 0x0A);
    //io_outb(0x3D5, (io_inb(0x3D5) & 0xC0) | cursor_start);
    io_outb(0x3D5, cursor_start);

    io_outb(0x3D4, 0x0B);
    //io_outb(0x3D5, (io_inb(0x3D5) & 0xE0) | cursor_end);
    io_outb(0x3D5, cursor_end);
}

void terminal_initialize(size_t width, size_t height, void* buffer) {
    VGA_WIDTH = width;
    VGA_HEIGHT = height;

    // Set our copy of the cursor to upper left
    terminal_row = 0;
    terminal_column = 0;
    
    terminal_buffer = (uint16_t*) buffer; // Set buffer to start of framebuffer

    terminal_fg = VGA_DEFAULT_FG;
    terminal_bg = VGA_DEFAULT_BG;

    // Loop through every cell and clear it
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', vga_color(VGA_DEFAULT_FG, VGA_DEFAULT_BG));
        }
    }

    enable_cursor(0, 15);
}

inline void terminal_move_cursor(size_t x, size_t y) {

    uint16_t pos = y * VGA_WIDTH + x;

    io_outb(0x3D4, 0x0F);
    io_outb(0x3D5, (uint8_t) (pos & 0xFF));

    io_outb(0x3D4, 0x0E);
    io_outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void terminal_scroll_down(void) {
    terminal_row = VGA_HEIGHT - 1;

    for (size_t y = 0; y < VGA_HEIGHT-1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index_cur = y * VGA_WIDTH + x;
            const size_t index_next = index_cur + VGA_WIDTH;
            terminal_buffer[index_cur] = terminal_buffer[index_next];
        }
    }

    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = (VGA_HEIGHT-1) * VGA_WIDTH + x;
        terminal_buffer[index] = vga_entry(' ', vga_color(terminal_fg, terminal_bg));
    }
}

void terminal_writechar(const char c, bool updateCursor) {

    if (c == '\0') {
        return;
    }

    // Newline
    else if (c == '\n') {
        terminal_column = 0;
        terminal_row++;

        if (terminal_row >= VGA_HEIGHT) {
            terminal_scroll_down();
        }

        if (updateCursor) {
            terminal_move_cursor(terminal_column, terminal_row);
        }
        return;
    }

    // Backspace
    else if (c == '\b') {
        terminal_column--;

        if (updateCursor) {
            terminal_move_cursor(terminal_column, terminal_row);
        }
        return;
    }

    // Right arrow
    else if (c == 26) {
        terminal_column++;

        if (updateCursor) {
            terminal_move_cursor(terminal_column, terminal_row);
        }
        return;
    }

    // Escape
    else if (c == 27) {
        escapeIndex = 0;
        insideEscape = true;
        setBackground = false;
        return;
    }

    if (insideEscape) {

        if (c == '[') {
            if (escapeIndex != 0) {
                insideEscape = false;
            } else {
                escapeIndex++;
            }
            return;
        }

        // Foreground
        else if (escapeIndex == 1) {
            int fg;

            if ('0' <= c && c <= '9') {
                fg = c - '0';
            } else if ('a' <= c && c <= 'f') {
                fg = c - 'a' + 10;
            } else {
                insideEscape = false;
                return;
            }

            terminal_fg = fg;
            escapeIndex++;
            return;
        }

        else if (c == ';') {
            escapeIndex++;
            return;
        }

        // Background
        else if (escapeIndex == 3) {
            int bg;

            if ('0' <= c && c <= '9') {
                bg = c - '0';
            } else if ('a' <= c && c <= 'f') {
                bg = c - 'a' + 10;
            } else {
                insideEscape = false;
                return;
            }

            terminal_bg = bg;
            escapeIndex++;
            return;
        }

        // m
        else if (c == 'm') {
            if (terminal_fg == 0) {
                terminal_fg = VGA_DEFAULT_FG;
                terminal_bg = VGA_DEFAULT_BG;
            }
            insideEscape = false;
            return;
        }

    }

    const size_t index = terminal_row * VGA_WIDTH + terminal_column;
    terminal_buffer[index] = vga_entry(c, vga_color(terminal_fg, terminal_bg));

    terminal_column++;
    if (terminal_column >= VGA_WIDTH) {
        terminal_column = 0;
        terminal_row++;

        if (terminal_row >= VGA_HEIGHT) {
            terminal_scroll_down();
        }
    }

    if (updateCursor) {
        terminal_move_cursor(terminal_column, terminal_row);
    }
}

void terminal_write(const char* string, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_writechar(string[i], false);
    }

    terminal_move_cursor(terminal_column, terminal_row);
}

void terminal_writestring(const char* string) {
    size_t i = 0;
    while (string[i] != 0) {
        terminal_writechar(string[i], false);
        i++;
    }

    terminal_move_cursor(terminal_column, terminal_row);
}

