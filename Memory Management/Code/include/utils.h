/**
 * @file utils.h
 * @brief Contains useful macros and utilities to be used by the memory allocator.
 * @version 0.1
 * @date 2023-06-03
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef UTILS_H
#define UTILS_H

#include "log.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((b) < (a) ? (a) : (b))

// we specify that the strings in our program won't exceed length of 100 characters
#define MAX_STRING_LENGTH 1024

// macros for pointer manipulation
#define PTR_ADD(ptr, offset) ((void*)((char*)(ptr) + (offset)))
#define PTR_SUB(ptr, offset) ((void*)((char*)(ptr) - (offset)))

// macro for checking if a pointer is 'align' bytes aligned
#define IS_ALIGNED(ptr, align) (((uintptr_t)(const void*)(ptr)) % (align) == 0)

// copy string
#define COPY(str) (str ? strndup(str, MAX_STRING_LENGTH) : NULL)

#endif // UTILS_H