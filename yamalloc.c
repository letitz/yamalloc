/*
 * Yet Another Malloc
 * yamalloc.c
 */

/*---------------------*/
/* Feature test macros */
/*---------------------*/

#define _DEFAULT_SOURCE // for sbrk

/*----------*/
/* Includes */
/*----------*/

#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

#include "yamalloc.h"
#include "ya_debug.h"
#include "ya_block.h"

/*-----------*/
/* Constants */
/*-----------*/


/*--------*/
/* Macros */
/*--------*/


/*--------------------*/
/* Local declarations */
/*--------------------*/

intptr_t *heap_init();
intptr_t *heap_extend(intptr_t size);

/*----------------------*/
/* Function definitions */
/*----------------------*/

#ifdef YA_DEBUG
/* Print all blocks in the heap */
void ya_print_blocks() {
    block_print_range(heap_start, heap_end);
}
#endif


/* Allocates enough memory to store at least size bytes.
 * Returns a dword-aligned pointer to the memory or NULL in case of failure. */
void *malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    if (heap_start == NULL || heap_end == NULL) {
        if (!heap_init()) {
            return NULL;
        }
    }
    intptr_t size_w = block_fit(size);
    intptr_t *block = block_find(size_w);
    intptr_t block_size = YA_SZ_BLK(block);
    if (size_w < block_size) {
        block_split(block, size_w);
    }
    block_alloc(block);
    return block;
}

/* Frees the memory block pointed to by ptr, which must have been allocated
 * through a call to malloc, calloc or realloc before. Otherwise, undefined
 * behavior occurs. */
void free(void *ptr) {
    intptr_t *block = ptr;
    if (block < heap_start || block > heap_end || !YA_IS_ALLOC_BLK(block)) {
        return; // TODO: provoke segfault
    }
    block_free(block);
    block_join(block);
}

/* Allocates enough memory to store an array of nmemb elements,
 * each size bytes large, and clears the memory.
 * Returns the pointer to the allocated memory or NULL in case of failure. */
void *calloc(size_t nmemb, size_t size) {
    intptr_t *block = malloc(size * nmemb);
    intptr_t block_size = YA_SZ_BLK(block);
    for (int i = 0; i < block_size - 2; i++) {
        block[i] = 0;
    }
    return block;
}

/* Resizes the previously allocated memory pointed to by ptr to size.
 * If ptr is NULL, it is equivalent to malloc(size).
 * If called with size 0, it is equivalent to free(ptr).
 * If ptr does not point to memory previously allocated by malloc, calloc or
 * realloc, undefined behavior occurs.
 *  */
void *realloc(void *ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
    }
    intptr_t *block = ptr;
    if (block < heap_start) {
        return NULL; // TODO: provoke segfault
    }
    intptr_t size_w = block_fit(size);
    intptr_t block_size = YA_SZ_BLK(block); // segfault if ptr after heap end
    if (size_w <= block_size) {
        intptr_t *next = block_split(block, size_w);
        if (next) {
            block_join_next(next); // coalesce the leftovers
        }
        block_alloc(block);
        return block;
    }
    intptr_t *next = block + block_size;
    // try to use next free block
    if (next < heap_end) {
        intptr_t next_size = YA_SZ_BLK(next);
        if (!YA_IS_ALLOC_BLK(next) && size_w <= block_size + next_size) {
            block_join_next(block); // coalesce
            block_split(block, size_w); // split if possible
            // no need to coalesce 
            block_alloc(block); // mark block as allocated
            return block;
        }
    }
    // resizing failed, so allocate a whole new block and copy
    // could handle the case when the block is the last in the heap 
    // a bit more gracefully and just grow the heap instead of copying
    intptr_t *new_block = malloc(size);
    for (int i = 0; i < block_size; i++) {
        new_block[i] = block[i];
    }
    free(block);
    return new_block;
}
