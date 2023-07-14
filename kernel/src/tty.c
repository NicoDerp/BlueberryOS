
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/tty.h>
#include <kernel/memory.h>
#include <kernel/io.h>

#include <bits/tty.h>
#include <stdio.h>
#include <string.h>


static size_t VGA_WIDTH = 80;
static size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;

uint16_t* terminal_buffer;

uint8_t terminal_fg;
uint8_t terminal_bg;

char insideEscape = false;
uint32_t escapeIndex = 0;
bool privateMode = false;

#define BUF_SIZE  8
#define BUF_COUNT 4

char argBuf[BUF_COUNT][BUF_SIZE];
uint32_t argIndex = 0;

size_t saved_terminal_row;
size_t saved_terminal_column;
uint8_t saved_terminal_fg;
uint8_t saved_terminal_bg;
uint16_t* savedScreen = NULL;

inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t) color << 8 | (uint16_t) c;
}

inline uint8_t vga_color(uint8_t fg, uint8_t bg) {
    return bg << 4 | fg;
}

static inline void terminal_clear(void) {

    // Loop through every cell and clear it
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', vga_color(VGA_DEFAULT_FG, VGA_DEFAULT_BG));
        }
    }
}

static inline void terminal_clear_row(void) {

    size_t index = terminal_row * VGA_WIDTH;
    for (size_t i = 0; i < VGA_WIDTH; i++) {
        terminal_buffer[index + i] = vga_entry(' ', vga_color(VGA_DEFAULT_FG, VGA_DEFAULT_BG));
    }
    terminal_column = 0;
}

static inline void terminal_save_screen(void) {

    if (savedScreen == NULL)
        savedScreen = (uint16_t*) kmalloc(sizeof(uint16_t) * VGA_WIDTH * VGA_HEIGHT);

    memcpy(savedScreen, terminal_buffer, sizeof(uint16_t) * VGA_WIDTH * VGA_HEIGHT);

    saved_terminal_row = terminal_row;
    saved_terminal_column = terminal_column;
    saved_terminal_fg = terminal_fg;
    saved_terminal_bg = terminal_bg;
}

static inline void terminal_restore_screen(void) {

    if (savedScreen == NULL)
        return;

    memcpy(terminal_buffer, savedScreen, sizeof(uint16_t) * VGA_WIDTH * VGA_HEIGHT);

    terminal_row = saved_terminal_row;
    terminal_column = saved_terminal_column;
    terminal_fg = saved_terminal_fg;
    terminal_bg = saved_terminal_bg;
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

    terminal_clear();
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

void parseArgs(char type) {

    if (type == 'm') {

        int fg = atoi(argBuf[0]);

        // <n>m
        if (escapeIndex == 1) {
            if (fg != 0)
                return;

            terminal_fg = VGA_DEFAULT_FG;
            terminal_bg = VGA_DEFAULT_BG;

            return;
        }

        int bg = atoi(argBuf[1]);

        if (terminal_fg == 0) {
            terminal_fg = VGA_DEFAULT_FG;
        } else {
            terminal_fg = fg - 30;
        }

        if (terminal_bg == 0) {
            terminal_bg = VGA_DEFAULT_BG;
        } else {
            terminal_bg = bg - 46;
        }

    }
    else if (type == 'J') {

        int num = atoi(argBuf[0]);

        /* \e[2J -> Erase entire screen */
        if (num == 2) {
            terminal_clear();
            terminal_row = 0;
            terminal_column = 0;
        }
    }
    else if (type == 'K') {

        int num = atoi(argBuf[0]);

        /* \e[2K -> Erase the entire line */
        if (num == 2) {
            terminal_clear_row();
        }
    }
    else if (privateMode) {

        int num = atoi(argBuf[0]);

        /* \e[?47h -> Save screen */
        if (num == 47 && type == 'h') {
            terminal_save_screen();
        }

        /* \e[?47l -> Restore screen */
        else if (num == 47 && type == 'l') {
            terminal_restore_screen();
        }
    }
}

int terminal_execute_cmd(int cmd, int* args, unsigned int** ret) {

    switch (cmd) {

        /* Color commands */
        case (TTY_CHANGE_COLOR):
            {
                if (args[0] == 0)
                    terminal_fg = VGA_DEFAULT_FG;
                else
                    terminal_fg = args[0] - 30;

                if (args[1] == 0)
                    terminal_bg = VGA_DEFAULT_BG;
                else
                    terminal_bg = args[1] - 46;
            }
            break;

        /* Resets all modes like colors and styles */
        case (TTY_RESET_MODES):
            {
                /* \e[0m */
                terminal_fg = VGA_DEFAULT_FG;
                terminal_bg = VGA_DEFAULT_BG;
            }
            break;

        /* Erase screen */
        case (TTY_ERASE_SCREEN):
            {
                terminal_clear();
                terminal_row = 0;
                terminal_column = 0;
            }
            break;

        /* Erase line */
        case (TTY_ERASE_LINE):
            {
                terminal_clear_row();
            }
            break;

        /* Save screen */
        case (TTY_SAVE_SCREEN):
            {
                terminal_save_screen();
            }
            break;

        /* Restore screen */
        case (TTY_RESTORE_SCREEN):
            {
                terminal_restore_screen();
            }
            break;

        /* Get cursor position */
        case (TTY_GET_CURSOR):
            {
                if (ret == NULL)
                    return -1;

                *ret[0] = terminal_row;
                *ret[1] = terminal_column;
            }
            break;

        /* Set cursor position */
        case (TTY_MOVE_CURSOR):
            {
                terminal_row = args[0];
                terminal_column = args[1];
                terminal_move_cursor(terminal_column, terminal_row);
            }
            break;

        /* Get max window size */
        case (TTY_GET_MAX_WIN_SIZE):
            {
                if (ret == NULL)
                    return -1;

                *ret[0] = VGA_WIDTH;
                *ret[1] = VGA_HEIGHT;
            }
            break;

        default:
            return -1;
            break;
    }

    return 0;
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

    // Escape
    else if (c == 27) {
        escapeIndex = 0;
        argIndex = 0;
        insideEscape = true;
        privateMode = false;
        return;
    }

    if (insideEscape) {

        if (c == '[') {

            if (escapeIndex != 0) {
                insideEscape = false;
            } else {
                escapeIndex++;
            }
        }

        else if (escapeIndex == 1 && c == '?') {
            privateMode = true;
        }

        else if (escapeIndex >= 1 && '0' <= c && c <= '9') {

            if (escapeIndex-1 >= BUF_COUNT) {

            } if (argIndex >= BUF_SIZE) {

            } else {
                argBuf[escapeIndex-1][argIndex++] = c;
            }
        }

        else if (c == ';') {

            if (escapeIndex-1 >= BUF_COUNT) {

            } else if (argIndex >= BUF_SIZE) {

            } else {
                argBuf[escapeIndex-1][argIndex] = '\0';
            }

            escapeIndex++;
            argIndex = 0;
        }

        else {
            argBuf[escapeIndex-1][argIndex] = '\0';
            parseArgs(c);
            insideEscape = false;
        }

        return;
    }

    const size_t index = terminal_row * VGA_WIDTH + terminal_column;
    terminal_buffer[index] = vga_entry(c, vga_color(terminal_fg, terminal_bg));

    terminal_column++;
    if (terminal_column > VGA_WIDTH) {
        terminal_column = 0;
        terminal_row++;

        if (terminal_row > VGA_HEIGHT) {
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

