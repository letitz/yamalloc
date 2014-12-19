/*
 * Yet Another Malloc
 * ya_freelist.h
 */

#ifndef YA_FREELIST_H
#define YA_FREELIST_H

/*----------*/
/* Includes */
/*----------*/

#include <stdint.h> // for intptr_t

#include "ya_block.h"

/*---------*/
/* Inlines */
/*---------*/

static inline intptr_t *fl_prev(intptr_t *block) {
    return (intptr_t *) block[-2];
}

static inline intptr_t *fl_next(intptr_t *block) {
    return (intptr_t *) block[block_size(block)-3];
}

static inline void fl_set_prev(intptr_t *block, intptr_t *prev) {
    block[-2] = (intptr_t) prev;   
}

static inline void fl_set_next(intptr_t *block, intptr_t *next) {
    block[block_size(block)-3] = (intptr_t) next;
}

/*--------------*/
/* Declarations */
/*--------------*/

#ifdef YA_DEBUG
void fl_debug_print();
#endif

/* Splices the allocated block out of the free list. */
void fl_alloc(intptr_t *block);

/* Adds the freed block to the start of the free list. */
void fl_free(intptr_t *block);

/* Returns the first block in the free list at min_size words long.
 * Returns NULL if no adequate block is found. Does not grow the heap. */
intptr_t *fl_find(intptr_t min_size);

/* Force split of block [block_size] into [size, block_size - size]. */
void fl_split(intptr_t *block, intptr_t size);

/* Joins are unnecessary because the pointers will still be ok after a join. */

#endif
