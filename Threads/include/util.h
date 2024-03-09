#ifndef UTIL_H
#define UTIL_H

#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <setjmp.h>

typedef unsigned long addr_t; // refers to addresses in our program

// Function to write a formatted string to the STDOUT descriptor
void write_formatted(const char *format, ...);

// output function for printing, default is printf
#define LOG_OUT(format, ...)                    \
    do                                          \
    {                                           \
        write_formatted(format, ##__VA_ARGS__); \
    } while (0)

addr_t mangle(addr_t addr);

#endif // UTIL_H