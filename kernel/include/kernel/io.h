
#ifndef KERNEL_IO_H
#define KERNEL_IO_H

#include <stdint.h>

void io_outb(uint16_t port, uint8_t val);
uint8_t io_inb(uint16_t port);
void io_wait(void);
void io_enable(void);

#endif /* KERNEL_IO_H */

