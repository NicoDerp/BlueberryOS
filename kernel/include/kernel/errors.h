
#ifndef KERNEL_ERRORS_H
#define KERNEL_ERRORS_H

extern __attribute__((noreturn)) void kabort();

void kerror(const char* msg);

#endif /* KERNEL_ERRORS_H */

