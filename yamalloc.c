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
    intptr_t size = block_fit(n_bytes);
    intptr_t *block = block_find(size);
    if (!block) {
        block = heap_extend(n_bytes);
    }
    if (block_split(block, size)) {
        // block was indeed split, so splt it in the free list as well
        fl_split(block, size);
    }
    block_alloc(block);
    fl_alloc(block);
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
    fl_free(block);
    fl_join(block);
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
        return NULL;
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
            fl_free(next); // add the next from the free list
            fl_join_next(next);
            block_join_next(next); // coalesce the leftovers
        }
        block_alloc(block);
        // no need to remove it from the free list, it wasn't in it
        return block;
    }
    intptr_t *next = block + size;
    if (next == heap_end ||
            (!block_is_alloc(next) && next + block_size(next) == heap_end)) {
        // grow the heap and extend block
        next = heap_extend(n_bytes);
        fl_free(block);
        fl_join_next(block);
        block_join_next(block);
        if (block_split(block, new_size)) {
            fl_split(block, new_size);
        }
        block_alloc(block);
        fl_alloc(block);
        return block;
    }
    // try to use next free block
    if (next < heap_end) {
        intptr_t next_size = block_size(next);
        if (!block_is_alloc(next) && new_size <= size + next_size) {
            // try to split the next block at the right size
            if (block_split(next, new_size - size)) {
                // split successful, must split in the free list too
                fl_split(block, new_size);
            }
            fl_alloc(next); // remove the next block from the free list
            fl_join_next(block);
            block_join_next(block); // coalesce
            block_alloc(block); // mark block as allocated
            return block;
        }
    }
    // resizing failed, so allocate a whole new block and copy
    intptr_t *new_block = malloc(n_bytes);
    for (int i = 0; i < size; i++) {
        new_block[i] = block[i];
    }
    free(block);
    return new_block;
}

#ifdef YA_DEBUG
/* Print all blocks in the heap */
void ya_print_blocks() {
    ya_debug("All blocks:\n");
    block_print_range(heap_start, heap_end);
    ya_debug("Free blocks:\n");
    fl_debug_print();
}

/* Checks internal state for errors.
 * Returns -1 on error, 0 otherwise. */
int ya_check() {
    int heap_free = heap_check();
    if (heap_free == -1) {
        return -1;
    }
    int fl_free = fl_check();
    if (fl_free == -1) {
        return -1;
    }
    if (fl_free != heap_free) {
        ya_debug("ya_check: heap_check reports %d free blocks, fl_check %d\n",
                heap_free, fl_free);
        return -1;
    }
    return 0;
}

#endif
