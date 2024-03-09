#include "core_mem.h"
#include "mm_lib.h"
#include "utils.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <stdbool.h>

// -------- Macros defined for the allocator --------
#define ALIGNMENT sizeof(void *)
#define MIN_BLOCK_SIZE sizeof(struct block_header)

// --------- Definitions of the headers ---------
struct block_header
{
    size_t size;
    struct block_header *next;
};
struct block_header *free_list = NULL;

// --------- Helper function declarations ---------
static void split_block(struct block_header *block, size_t size)
{
    size_t remaining_space = block->size - size;
    if (remaining_space >= MIN_BLOCK_SIZE)
    {
        struct block_header *new_block = (struct block_header *)PTR_ADD(block, size);
        new_block->size = remaining_space;
        new_block->next = block->next;
        block->size = size;
        block->next = new_block;
    }
}

static size_t align(size_t size)
{
    size_t remainder = size % ALIGNMENT;
    if (remainder != 0)
    {
        size += ALIGNMENT - remainder;
    }

    if (size < MIN_BLOCK_SIZE)
    {
        size = MIN_BLOCK_SIZE;
    }

    return size;
}

// --------- Function Definitions ---------
void mm_init()
{
    void *memory = cm_sbrk(MIN_BLOCK_SIZE + sizeof(struct block_header));
    struct block_header *block = (struct block_header *)memory;
    block->size = MIN_BLOCK_SIZE;
    block->next = NULL;
    free_list = block;
}

void *mm_malloc(size_t size)
{
    size_t aligned_size = align(size + sizeof(struct block_header));
    char *search_scheme = getenv("SEARCH_SCHEME");

    struct block_header *current = free_list;
    struct block_header *previous = NULL;

    if (strcmp(search_scheme, "FIRST_FIT") == 0)
    {
        struct block_header *first_fit = NULL;
        while (current != NULL)
        {
            if (current->size >= aligned_size)
            {
                first_fit = current;

                if (first_fit != NULL)
                {
                    split_block(first_fit, aligned_size);

                    if (previous == NULL)
                    {
                        free_list = first_fit->next;
                    }
                    else
                    {
                        previous->next = first_fit->next;
                    }
                }
                return PTR_ADD(first_fit, sizeof(struct block_header));
            }
            previous = current;
            current = current->next;
        }
    }

    else if (strcmp(search_scheme, "BEST_FIT") == 0)
    {
        struct block_header *best_fit = NULL;
        struct block_header *best_fit_prev = NULL;
        while (current != NULL)
        {
            if (current->size >= aligned_size)
            {
                if (best_fit == NULL || current->size < best_fit->size)
                {
                    best_fit = current;
                    best_fit_prev = previous;
                }
            }
            previous = current;
            current = current->next;
        }

        if (best_fit != NULL)
        {
            split_block(best_fit, aligned_size);

            if (best_fit_prev == NULL)
            {
                free_list = best_fit->next;
            }
            else
            {
                best_fit_prev->next = best_fit->next;
            }
            return PTR_ADD(best_fit, sizeof(struct block_header));
        }
    }

    else if (strcmp(search_scheme, "WORST_FIT") == 0)
    {
        struct block_header *worst_fit = NULL;
        struct block_header *worst_fit_prev = NULL;

        while (current != NULL)
        {
            if (current->size >= aligned_size)
            {
                if (worst_fit == NULL || current->size > worst_fit->size)
                {
                    worst_fit = current;
                    worst_fit_prev = previous;
                }
            }

            previous = current;
            current = current->next;
        }

        if (worst_fit != NULL)
        {
            split_block(worst_fit, aligned_size);

            if (worst_fit_prev == NULL)
            {
                free_list = worst_fit->next;
            }
            else
            {
                worst_fit_prev->next = worst_fit->next;
            }

            return PTR_ADD(worst_fit, sizeof(struct block_header));
        }
    }

    void *memory = cm_sbrk(MAX(aligned_size, MIN_BLOCK_SIZE));
    struct block_header *block = (struct block_header *)memory;
    block->size = MAX(aligned_size, MIN_BLOCK_SIZE);
    block->next = NULL;

    return PTR_ADD(block, sizeof(struct block_header));
}

void mm_free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    struct block_header *block = (struct block_header *)PTR_SUB(ptr, sizeof(struct block_header));

    struct block_header *current = free_list;
    struct block_header *previous = NULL;

    while (current != NULL)
    {
        if (current > block)
        {
            block->next = current;
            if (previous == NULL)
            {
                free_list = block;
            }
            else
            {
                previous->next = block;
            }
            break;
        }
        else
        {
            previous = current;
            current = current->next;
        }
    }

    if (current == NULL)
    {
        block->next = NULL;
        if (previous == NULL)
        {
            free_list = block;
        }
        else
        {
            previous->next = block;
        }
    }

    current = free_list;
    while (current != NULL && current->next != NULL)
    {
        if (current->next != PTR_ADD(current, current->size))
        {
            current = current->next;
        }
        else
        {
            current->size = current->size + current->next->size;
            current->next = current->next->next;
        }
    }
}

void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
    {
        return mm_malloc(size);
    }

    if (size == 0)
    {
        mm_free(ptr);
        return NULL;
    }

    struct block_header *block = (struct block_header *)PTR_SUB(ptr, sizeof(struct block_header));
    size_t aligned_size = align(size + sizeof(struct block_header));

    if (aligned_size <= block->size)
    {
        return ptr;
    }
    else
    {
        void *new_ptr = mm_malloc(size);
        memcpy(new_ptr, ptr, MIN(size, block->size));
        mm_free(ptr);
        return new_ptr;
    }
}