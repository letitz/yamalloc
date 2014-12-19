/*
 * Yet Another Malloc
 * ya_block.c
 * Defines operations on blocks and boundary tags
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

#include "ya_debug.h"
#include "ya_block.h"

/*---------*/
/* Globals */
/*---------*/

intptr_t *heap_start = NULL; // with space for 2 words before
intptr_t *heap_end = NULL;   // first block outside heap

/*----------------------*/
/* Function definitions */
/*----------------------*/

#ifdef YA_DEBUG
void block_print_range(intptr_t *start, intptr_t *end) {
    if (!start || !end) {
        return;
    }
    intptr_t *block;
    intptr_t size;
    for (block = start; block < end; block += size) {
        size = block_size(block);
        printf("Block at %p  size = %ld  alloc = %d\n",
                block, size, block_is_alloc(block));
    }
}
#endif

/* Initializes the block's boundary tags. */
void block_init(intptr_t *block, intptr_t size) {
    block[-1]       = size;
    block[size - 2] = size;
}

/* Sets the allocated bit in the block's boundary tags. */
void block_alloc(intptr_t *block) {
    intptr_t size = block_size(block);
    block[-1]     |= 1;
    block[size-2] |= 1;
}

/* Erases the allocated bit in the block's boundary tags. */
void block_free(intptr_t *block) {
    intptr_t size = block_size(block);
    block[-1]     &= -2;
    block[size-2] &= -2;
}

/* Fills block with zeros. */
void block_clear(intptr_t *block) {
    intptr_t *end = block + block_size(block) - 2;
    for (intptr_t *p = block; p < end; p++) {
        *p = 0;
    }
}

/* Returns the size in words of the smallest block that can
 * store n_bytes bytes. Takes alignment and boundary tags into account */
intptr_t block_fit(size_t n_bytes) {
    intptr_t n_words = YA_ROUND_DIV(n_bytes, YA_WORD_SZ); // size in words
    // round to dword and make space for tags 
    intptr_t size = 2 + YA_ROUND(n_words, 2);
    ya_debug("block_fit: requested = %ld, allocating = %ld * %ld = %ld\n",
            n_bytes, size, YA_WORD_SZ, size * YA_WORD_SZ);
    return size;
}

/* Tries to coalesce a block with its previous neighbor.
 * Returns a pointer to the coalesced block. */
intptr_t *block_join_prev(intptr_t *block) {
    if (block < heap_start + YA_BLK_MIN_SZ) {
        return block; // there cannot be a previous block
    }
    intptr_t prev_size = tag_size(block[-2]);
    intptr_t *prev = block - prev_size;
    if (prev <= heap_start || block_is_alloc(prev)) {
        return block;
    }
    intptr_t size = block_size(block);
    block_init(prev, prev_size + size);
    ya_debug("block_join_prev: joining %p:%ld and %p:%ld -> %p:%ld\n",
            block, size, prev, prev_size, prev, prev_size + size);
    return prev;
}

/* Tries to colesce a block with its next neighbor.
 * Returns the unchanged pointer to the block. */
intptr_t *block_join_next(intptr_t *block) {
    intptr_t size = block_size(block);
    intptr_t *next = block + size;
    if (next >= heap_end || block_is_alloc(next)) {
        return block;
    }
    intptr_t next_size = block_size(next);
    block_init(block, size + next_size);
    ya_debug("block_join_next: joining %p:%ld and %p:%ld -> %p:%ld\n",
            block, size, next, next_size, block, size + next_size);
    return block;
}

/* Tries to coalesce a block with its previous and next neighbors.
 * Returns a pointer to the coalesced block. */
intptr_t *block_join(intptr_t *block) {
    block = block_join_prev(block);
    return block_join_next(block);
}

/* Split the block [block_size] into [size, block_size - size] if possible
 * Returns a pointer to the second block or NULL if no split occurred. */
intptr_t *block_split(intptr_t *block, intptr_t size) {
    intptr_t next_size = block_size(block) - size;
    if (next_size < YA_BLK_MIN_SZ) {
        return NULL; // not enough space to warrant a split
    }
    block_init(block, size);
    block_init(block + size, next_size);
    return block + size;
}

/* Try to find a free block at least size words big by walking the boundary
 * tags. If no block is found the heap is grown adequately.
 * Returns a pointer to the block or NULL in case of failure. */
intptr_t *block_find(intptr_t min_size) {
    intptr_t *block;
    intptr_t size;
    for (block = heap_start; block < heap_end; block += size) {
        size = block_size(block);
        if (!block_is_alloc(block) && min_size <= size) {
            return block;
        }
    }
    // could not find block, extend heap
    return heap_extend(min_size);
}

/* Initializes the heap by calling sbrk to allocate some starter memory.
 * Sets heap_start and heap_end to their appropriate values.
 * Returns the pointer to the start of the heap or NULL in case of failure. */
intptr_t *heap_init() {
    intptr_t size_w = YA_CHUNK_SZ / YA_WORD_SZ;
    void *ptr = sbrk(YA_WORD_SZ * (size_w + 2));
    if (ptr == (void*) -1) {
        heap_start = NULL;
        heap_end = NULL;
        return NULL;
    }
    heap_start     = ptr; // cast to intptr_t *
    heap_start    += 2; // space for the first block[-1] + dword alignment
    heap_end       = heap_start + size_w;
    block_init(heap_start, size_w);
    ya_debug("heap_init: start = %p, end = %p, size_w = %ld\n",
            heap_start, heap_end, size_w);
    return heap_start;
}

/* Extends the heap by at least size_w words by calling sbrk.
 * Returns a pointer to the last (free) block or NULL in case of failure. */
intptr_t *heap_extend(intptr_t size_w) {
    intptr_t size = YA_ROUND(size_w * YA_WORD_SZ, YA_CHUNK_SZ);
    size_w = size / YA_WORD_SZ;
    ya_debug("heap_extend: size_w = %ld\n", size_w);
    void *ptr = sbrk(size);
    if (ptr == (void *) - 1) {
        return NULL;
    }
    intptr_t *block = ptr; // == old heap_end
    heap_end        = block + size_w;
    block_init(block, size_w);
    ya_debug("heap_extend: old end = %p, new end = %p, size_w = %ld\n",
            block, heap_end, size_w);
    ya_print_blocks();
    return block_join(ptr);
}
