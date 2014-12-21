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

/* Returns a pointer to the first free block. */
intptr_t *fl_get_start() {
    return fl_start;
}

/* Returns a pointer to the last free block. */
intptr_t *fl_get_end() {
    return fl_end;
}

#ifdef YA_DEBUG

void fl_debug_print() {
    for (intptr_t *block = fl_start; block != NULL; block = fl_next(block)) {
        ya_debug("%p:%ld\n", block, block_size(block));
    }
}

/* Checks that block's information is consistent. The previous pointer should
 * point to correct_prev, which was the previous free block during free list
 * iteration.
 * Returns -1 on error, 0 otherwise. */
int fl_check_one(intptr_t *block, intptr_t *correct_prev) {
    if (block < heap_start || block >= heap_end) {
        ya_debug("fl_check_one: block %p out of bounds\n", block);
        return -1;
    }
    intptr_t *prev = fl_prev(block);
    if (prev && (prev < heap_start || prev >= heap_end)) {
        ya_debug("fl_check_one: previous pointer %p out of bounds [%p,%p[\n",
                prev, heap_start, heap_end);
        return -1;
    }
    if (correct_prev != prev) {
        ya_debug("fl_check_one(%p): previous pointer mismatch, "
                "should be %p, not %p\n", block, correct_prev, prev);
        return -1;
    }
    intptr_t *next = fl_next(block);
    if (next && (next < heap_start || next >= heap_end)) {
        ya_debug("fl_check_one: next pointer %p out of bounds [%p,%p[\n",
                next, heap_start, heap_end);
        return -1;
    }
    return 0;
}

/* Checks the free list for consistency.
 * Returns -1 on error, the total number of free blocks otherwise. */
int fl_check() {
    if (!fl_start) {
        if (fl_end) {
            ya_debug("fl_check: fl_start == NULL but fl_end == %p\n", fl_end);
            return -1;
        }
        return 0;
    }
    if (!fl_end) {
        ya_debug("fl_check: fl_end == NULL but fl_start == %p\n", fl_start);
        return -1;
    }
    if (fl_start < heap_start || fl_start >= heap_end) {
        ya_debug("fl_check: fl_start %p out of bounds\n", fl_start);
        return -1;
    }
    if (fl_end < heap_start || fl_end >= heap_end) {
        ya_debug("fl_check: fl_ebd %p out of bounds\n", fl_end);
        return -1;
    }
    int num_free = 0;
    intptr_t *prev = NULL;
    intptr_t *block;
    for (block = fl_start; block; block = fl_next(block)) {
        num_free++;
        if (fl_check_one(block, prev)) {
            return -1;
        }
        prev = block;
    }
    return num_free;
}

#endif // def YA_DEBUG

