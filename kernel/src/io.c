
#include <kernel/io.h>


void io_outb(uint16_t port, uint8_t val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

uint8_t io_inb(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %1, %0"
                  : "=a"(ret)
                  : "Nd"(port));
    return ret;
}

void io_wait(void)
{
    io_outb(0x80, 0);
}

void io_enable(void) {
    asm volatile("sti");
}

