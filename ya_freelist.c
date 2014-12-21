/*
 * Yet Another Malloc
 * ya_freelist.c
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
        ya_debug("%p:%ld\n", block, block_size(block));
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
    if (!fl_start && !fl_end) {
        // add to empty list
        fl_set_prev(block, NULL);
        fl_set_next(block, NULL);
        fl_start = block;
        fl_end = block;
        return;
    }
    if (block < fl_start) {
        // preprend block to the free list
        fl_set_prev(block, NULL);
        fl_set_next(block, fl_start);
        fl_set_prev(fl_start, block);
        fl_start = block;
        return;
    }
    if (block > fl_end) {
        // append block to the free list
        fl_set_prev(block, fl_end);
        fl_set_next(block, NULL);
        fl_set_next(fl_end, block);
        fl_end = block;
        return;
    }
    // splice the block in the middle of the list
    intptr_t *next;
    for (next = fl_start; next && next < block; next = fl_next(next)) {
        // *whistle*
    }
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
    if (block == fl_end) {
        fl_end = block+size;
    }
    fl_set_prev(block+size, block);
}

void fl_join_next(intptr_t *block) {
    intptr_t *next = block + block_size(block);
    if (fl_next(block) == next) {
        // skip a block
        ya_debug("fl_join_next: %p:%ld + %p:%ld -> %p:%ld\n",
                block, block_size(block), next, block_size(next),
                block, block_size(block) + block_size(next));
        fl_set_next(block, fl_next(next));
        fl_set_prev(next, fl_prev(block));
        if (fl_next(next) == NULL) {
            fl_end = block;
        }
    }
}

void fl_join_prev(intptr_t *block) {
    intptr_t *free_prev = fl_prev(block);
    if (!free_prev) {
        return;
    }
    // will not segfault because there is at least one block preceding
    intptr_t *prev = block - block[-4];
    if (prev == free_prev) {
        ya_debug("fl_join_prev: %p:%ld + %p:%ld -> %p:%ld\n",
                block, block_size(block), prev, block_size(prev),
                prev, block_size(block) + block_size(prev));
        fl_set_prev(block, fl_prev(prev));
        fl_set_next(prev, fl_next(block));
        if (fl_next(block) == NULL) {
            fl_end = prev;
        }
    }
}

void fl_join(intptr_t *block) {
    fl_join_next(block);
    fl_join_prev(block);
}
