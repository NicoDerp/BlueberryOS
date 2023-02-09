
#ifndef KERNEL_TTY_H
#define KERNEL_TTY_H

#include <stddef.h>

void terminal_initialize(void);
void terminal_write(const char* data, size_t size);
void terminal_writechar(char c);
void temrinal_writestring(const char* data);

#endif /* KERNEL_TTY_H */

