/*
 * Yet Another Malloc
 * ya_freelist.c
 */

/* Block layout:
 *
 * -2     -1     0                               size-4 size-3
 * +------+------+-------- - - - - - - - --------+------+------+
 * | prev | size | data...                       | size | next |
 * +------+------+-------- - - - - - - - --------+------+------+
 *
 */

/*----------*/
/* Includes */
/*----------*/

#include <stdio.h>

#include "ya_freelist.h"
#include "ya_block.h"

/*---------*/
/* Globals */
/*---------*/

intptr_t *fl_start = NULL; // sorted increasing
intptr_t *fl_end = NULL;   // sorted decreasing

/*-----------*/
/* Functions */
/*-----------*/

#ifdef YA_DEBUG
void fl_debug_print() {
    for (intptr_t *block = fl_start; block != NULL; block = fl_next(block)) {
        printf("Free block %p:%ld\n", block, block_size(block));
    }
}
#endif

/* Splices the allocated block out of the free list. */
void fl_alloc(intptr_t *block) {
    intptr_t size  = block_size(block);
    intptr_t *prev = fl_prev(block);
    intptr_t *next = fl_next(block);
    fl_set_prev(block, NULL);
    fl_set_next(block, NULL);
    if (prev) {
        fl_set_next(prev, next);
    } else {
        fl_start = next;
    }
    if (next) {
        fl_set_prev(next, prev);
    } else {
        fl_end = prev;
    }
}

/* Adds the freed block to the appropriate in the free list, which is kept
 * in sorted increasing order. */
void fl_free(intptr_t *block) {
    intptr_t *next;
    for (next = fl_start; next && block < next; next = fl_next(next)) {
        // *whistle*
    }
    if (next == fl_start) {
        // preprend block to the free list
        fl_set_prev(block, NULL);
        fl_set_next(block, fl_start);
        fl_set_prev(fl_start, block);
        fl_start = block;
        return;
    }
    if (!next) {
        // append block to the free list
        fl_set_prev(block, fl_end);
        fl_set_next(block, NULL);
        fl_set_next(fl_end, block);
        fl_end = block;
        return;
    }
    // splice the block in the middle of the list
    intptr_t *prev = fl_prev(next);
    fl_set_prev(block, prev);
    fl_set_next(block, next);
    fl_set_prev(next, block);
    fl_set_next(prev, block);
}

/* Returns the first block in the free list at min_size words long.
 * Returns NULL if no adequate block is found. Does not grow the heap. */
intptr_t *fl_find(intptr_t min_size) {
    for (intptr_t *block = fl_start; block != NULL; block = fl_next(block)) {    
        if (min_size <= block_size(block)) {
            return block;
        }
    }
    return NULL;
}

/* Force split of free block [block_size] into [size, block_size - size]. */
void fl_split(intptr_t *block, intptr_t size) {
    fl_set_next(block, block+size);
    fl_set_prev(block+size, block);
}

