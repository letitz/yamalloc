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

#define YA_IS_FREE(block) (((block)[0] & 1) == 0)
#define YA_ROUND_DIV(n, m) (((n) + ((m)-1)) / (m))
#define YA_ROUND(n, m) (YA_ROUND_DIV(n,m) * (m))

/* Globals */

intptr_t *heap_start = NULL;
intptr_t *heap_end   = NULL;

/* Local declarations */

int heap_init();
int heap_extend();

void block_join(intptr_t *block);

/* Function definitions */

#ifdef YA_DEBUG
void ya_print_blocks() {
    for (intptr_t *ptr = heap_start; ptr < heap_end; ptr += ptr[0]) {
        fprintf(stderr, "Block at %p, size %ld\n", ptr, ptr[0]);
    }
}
#endif

void block_join_prev(intptr_t *block) {
    intptr_t *prev = block - block[-1];
    if (prev >= heap_start && YA_IS_FREE(prev)) {
        intptr_t size = prev[0] + block[0];
        prev[0] = size;
        block[block[0] - 1] = size;
    }
}

void block_join_next(intptr_t *block) {
    intptr_t *next = block + block[0];
    if (next < heap_end && YA_IS_FREE(next)) {
        intptr_t size = next[0] + block[0];
        block[0] = size;
        next[next[0] - 1] = size;
    }
}

void block_join(intptr_t *block) {
    if (block > heap_start) {
        block_join_prev(block);
    }
    block_join_next(block);
}

void block_split(intptr_t *block, intptr_t size) {
    intptr_t old_size = block[0];
    block[0] = size;
    block[size - 1] = size;
    block[size] = old_size - size;
    block[old_size - 1] = old_size - size;
}

int heap_init() {
    heap_start = sbrk(0);    
    if (heap_start == (void*) -1) {
        heap_start = NULL;
        heap_end = NULL;
        return -1;
    }
    heap_end = heap_start;
    return 0;
}

int heap_extend(intptr_t size_w) {
    fprintf(stderr, "heap_extend: old heap_start = %p, heap_end = %p\n",
            heap_start, heap_end);
    intptr_t size = YA_ROUND(size_w * YA_SZ_WORD, YA_SZ_CHUNK);
    size_w = size / YA_SZ_WORD;
    fprintf(stderr, "heap_extend: %ld bytes, %ld words\n", size, size_w);
    void *ptr = sbrk(size);
    if (ptr == (void *) - 1) {
        return -1;
    }
    fprintf(stderr, "heap_extend: %ld bytes, %ld words\n", size, size_w);
    heap_end[0] = size_w;
    heap_end[size_w - 1] = size_w;
    fprintf(stderr, "heap_extend: heap_end = %p, heap_end + size_w = %p\n",
            heap_end, heap_end + size_w);
    fprintf(stderr, "heap_extend: heap_end[0] = %ld, heap_end[size_w - 1] = %ld\n",
            heap_end[0], heap_end[size_w - 1]);
    heap_end += size_w;
    fprintf(stderr, "heap_extend: new heap_end = %p\n", heap_end);
    block_join(ptr);
    return 0;
}

void *malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    if (heap_start == NULL || heap_end == NULL) {
        if (heap_init() != 0) {
            return NULL;
        }
    }
    intptr_t size_w = YA_ROUND_DIV(size + 2, YA_SZ_WORD); // size in words
    intptr_t align_dw = YA_ROUND(size_w, 2); // round to dword
    intptr_t *block;
    for (block = heap_start; block < heap_end; block += block[0]) {
        if (YA_IS_FREE(block) && block[0] >= align_dw) {
            break;
        }
    }
    fprintf(stderr, "size_w = %ld, align_dw = %ld\n", size_w, align_dw);
    if (block >= heap_end) { // could not find block, extend heap
        if (heap_extend(align_dw) != 0) { 
            return NULL;
        }
        block = heap_end - heap_end[-1];
        if (!YA_IS_FREE(block) || block[0] < align_dw) {
            return NULL;
        }
    }
    if (block[0] > align_dw) {
        block_split(block, align_dw);
    }
    block[0] |= 1;
    return block + 1;
}

void free(void *ptr) {
}
