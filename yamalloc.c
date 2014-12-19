/*
 * Yet Another Malloc
 * yamalloc.c
 */

/*----------*/
/* Includes */
/*----------*/

#include "yamalloc.h"
#include "ya_debug.h"
#include "ya_block.h"
#include "ya_freelist.h"

/*----------------------*/
/* Function definitions */
/*----------------------*/

#ifdef YA_DEBUG
/* Print all blocks in the heap */
void ya_print_blocks() {
    ya_debug("All blocks:\n");
    block_print_range(heap_start, heap_end);
    ya_debug("Free blocks:\n");
    fl_debug_print();
}
#endif

/* Allocates enough memory to store at least size bytes.
 * Returns a dword-aligned pointer to the memory or NULL in case of failure. */
void *malloc(size_t n_bytes) {
    if (n_bytes == 0) {
        return NULL;
    }
    if (heap_start == NULL || heap_end == NULL) {
        if (!heap_init()) {
            return NULL;
        }
    }
    intptr_t min_size = block_fit(n_bytes);
    intptr_t *block = block_find(min_size);
    block_split(block, min_size);
    block_alloc(block);
    return block;
}

/* Frees the memory block pointed to by ptr, which must have been allocated
 * through a call to malloc, calloc or realloc before. Otherwise, undefined
 * behavior occurs. */
void free(void *ptr) {
    intptr_t *block = ptr;
    if (block < heap_start || block > heap_end || !block_is_alloc(block)) {
        return; // TODO: provoke segfault
    }
    block_free(block);
    block_join(block);
}

/* Allocates enough memory to store an array of nmemb elements,
 * each size bytes large, and clears the memory.
 * Returns the pointer to the allocated memory or NULL in case of failure. */
void *calloc(size_t nmemb, size_t n_bytes) {
    intptr_t *block = malloc(n_bytes * nmemb);
    intptr_t size = block_size(block);
    block_clear(block);
    return block;
}

/* Resizes the previously allocated memory pointed to by ptr to size.
 * If ptr is NULL, it is equivalent to malloc(size).
 * If called with size 0, it is equivalent to free(ptr).
 * If ptr does not point to memory previously allocated by malloc, calloc or
 * realloc, undefined behavior occurs.
 *  */
void *realloc(void *ptr, size_t n_bytes) {
    if (!ptr) {
        return malloc(n_bytes);
    }
    if (n_bytes == 0) {
        free(ptr);
    }
    intptr_t *block = ptr;
    if (block < heap_start) {
        return NULL; // TODO: provoke segfault
    }
    intptr_t new_size = block_fit(n_bytes);
    intptr_t size = block_size(block); // segfault if ptr after heap end
    if (new_size == size) {
        return ptr; // don't change anything
    }
    if (new_size < size) {
        intptr_t *next = block_split(block, new_size);
        if (next) {
            block_join_next(next); // coalesce the leftovers
        }
        block_alloc(block);
        return block;
    }
    intptr_t *next = block + size;
    // try to use next free block
    if (next < heap_end) {
        intptr_t next_size = block_size(next);
        if (!block_is_alloc(next) && new_size <= size + next_size) {
            block_join_next(block); // coalesce
            block_split(block, new_size); // split if possible
            // no need to coalesce 
            block_alloc(block); // mark block as allocated
            return block;
        }
    }
    // resizing failed, so allocate a whole new block and copy
    // could handle the case when the block is the last in the heap 
    // a bit more gracefully and just grow the heap instead of copying
    intptr_t *new_block = malloc(n_bytes);
    for (int i = 0; i < size; i++) {
        new_block[i] = block[i];
    }
    free(block);
    return new_block;
}
