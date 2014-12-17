/*
 * Yet Another Malloc
 * yamalloc.c
 */

/* Feature test macros */

#define _DEFAULT_SOURCE // for sbrk

/* Includes */

#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

#include "yamalloc.h"

/* Constants */

#define YA_SZ_WORD  (sizeof(intptr_t))
#define YA_SZ_DWORD (2 * YA_SZ_WORD)
#define YA_SZ_CHUNK 8192

/* Macros */

#define YA_IS_ALLOC(block) ((block)[-1] & 1)
#define YA_SZ_BLOCK(size)  ((size) & -2)
#define YA_ROUND_DIV(n, m) (((n) + ((m)-1)) / (m))
#define YA_ROUND(n, m)     (YA_ROUND_DIV(n,m) * (m))

/* Globals */

intptr_t *heap_start = NULL; // space for 2 words before
intptr_t *heap_end   = NULL; // first block outside heap

/* Local declarations */

intptr_t *heap_init();
intptr_t *heap_extend(intptr_t size);

intptr_t *block_join(intptr_t *block);

/* Function definitions */

#ifdef YA_DEBUG
void ya_print_blocks() {
    intptr_t *block;
    intptr_t block_size;
    for (block = heap_start; block < heap_end; block += block_size) {
        block_size = YA_SZ_BLOCK(block[-1]);
        fprintf(stderr, "Block at %p  size = %ld  alloc = %d\n",
                block, block_size, YA_IS_ALLOC(block));
    }
}
#endif

intptr_t *block_join_prev(intptr_t *block) {
    intptr_t prev_size = YA_SZ_BLOCK(block[-2]);
    intptr_t *prev = block - prev_size;
    if (prev <= heap_start || YA_IS_ALLOC(prev)) {
        return block;
    }
    intptr_t block_size = YA_SZ_BLOCK(block[-1]);
    intptr_t size = prev_size + block_size;
    prev[-1] = size;
    block[block_size - 2] = size;
    return prev;
}

intptr_t *block_join_next(intptr_t *block) {
    intptr_t block_size = YA_SZ_BLOCK(block[-1]);
    intptr_t *next = block + block_size;
    if (next >= heap_end || YA_IS_ALLOC(next)) {
        return block;
    }
    intptr_t next_size = YA_SZ_BLOCK(next[-1]);
    intptr_t size = next_size + block_size;
    block[-1] = size;
    next[next_size - 2] = size;
    return block;
}

intptr_t *block_join(intptr_t *block) {
    if (block > heap_start) {
        block = block_join_prev(block);
    }
    return block_join_next(block);
}

void block_split(intptr_t *block, intptr_t size) {
    intptr_t old_size = YA_SZ_BLOCK(block[-1]);
    block[-1]           = size;
    block[size - 2]     = size;
    block[size - 1]     = old_size - size;
    block[old_size - 2] = old_size - size;
}

intptr_t *heap_init() {
    intptr_t size_w = YA_SZ_CHUNK / YA_SZ_WORD;
    void *ptr = sbrk(YA_SZ_WORD * (size_w + 2));
    if (ptr == (void*) -1) {
        heap_start = NULL;
        heap_end = NULL;
        return NULL;
    }
    heap_start  = ptr;
    heap_start += 2; // space for the first block[-1] + dword alignment
    heap_end = heap_start + size_w;
    heap_start[-1] = size_w;
    heap_end[-2]   = size_w;
    return heap_start;
}

intptr_t *heap_extend(intptr_t size_w) {
    fprintf(stderr, "heap_extend: old heap_start = %p, heap_end = %p\n",
            heap_start, heap_end);
    intptr_t size = YA_ROUND(size_w * YA_SZ_WORD, YA_SZ_CHUNK);
    size_w = size / YA_SZ_WORD;
    fprintf(stderr, "heap_extend: %ld bytes, %ld words\n", size, size_w);
    void *ptr = sbrk(size);
    if (ptr == (void *) - 1) {
        return NULL;
    }
    intptr_t *block = ptr; // == heap_end
    heap_end = block + size_w;
    block[-1]    = size_w;
    heap_end[-2] = size_w;
    fprintf(stderr, "heap_extend: new heap_end = %p\n", heap_end);
    return block_join(ptr);
}

void *malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    if (heap_start == NULL || heap_end == NULL) {
        if (!heap_init()) {
            return NULL;
        }
    }
    intptr_t size_w = YA_ROUND_DIV(size, YA_SZ_WORD); // size in words
    fprintf(stderr, "size_w = %ld\n", size_w);
    size_w = 2 + YA_ROUND(size_w, 2); // round to dword and make space for tags
    fprintf(stderr, "size_w = %ld\n", size_w);
    intptr_t *block;
    intptr_t block_size;
    for (block = heap_start; block < heap_end; block += block_size) {
        block_size = YA_SZ_BLOCK(block[-1]);
        if (!YA_IS_ALLOC(block) && size_w <= block_size) {
            break;
        }
    }
    if (block >= heap_end) { // could not find block, extend heap
        block = heap_extend(size_w);
        if (!block) { 
            return NULL;
        }
        block_size = YA_SZ_BLOCK(block[-1]);
    }
    if (size_w < block_size) {
        block_split(block, size_w);
    }
    block[-1] |= 1;
    block[block_size - 2] |= 1;
    return block;
}

void free(void *ptr) {
    intptr_t *block = ptr;
    if (block < heap_start || block > heap_end || !YA_IS_ALLOC(block)) {
        return; // TODO: provoke segfault
    }
    intptr_t block_size = YA_SZ_BLOCK(block[-1]);
    block[-1]             &= -2; // erase allocated bit
    block[block_size - 2] &= -2;
    block_join(block);
}
