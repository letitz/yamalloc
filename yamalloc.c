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

#define YA_SZ_WORD  (sizeof(intptr_t)) // big enough to hold a pointer
#define YA_SZ_DWORD (2 * YA_SZ_WORD)   // storage is aligned to a dword
#define YA_SZ_CHUNK 8192               // request memory 8k by 8k from OS

/* Macros */

#define YA_IS_ALLOC_TAG(tag) ((tag) & 1)
#define YA_SZ_TAG(tag)       ((tag) & -2)

#define YA_IS_ALLOC_BLK(block) YA_IS_ALLOC_TAG((block)[-1])
#define YA_SZ_BLK(block)       YA_SZ_TAG((block)[-1])

#define YA_IS_ALLOC_END(b_end) YA_IS_ALLOC_TAG((b_end)[0])
#define YA_SZ_END(b_end)       YA_SZ_TAG((b_end)[0])

#define YA_ROUND_DIV(n, m) (((n) + ((m)-1)) / (m))
#define YA_ROUND(n, m)     (YA_ROUND_DIV(n,m) * (m))

/* Globals */

intptr_t *heap_start = NULL; // space for 2 words before
intptr_t *heap_end   = NULL; // first block outside heap

/* Local declarations */

intptr_t *heap_init();
intptr_t *heap_extend(intptr_t size);

intptr_t *block_join(intptr_t *block);
void block_split(intptr_t *block, intptr_t size);
intptr_t *block_find(intptr_t size);

/* Function definitions */

#ifdef YA_DEBUG
void ya_print_blocks() {
    intptr_t *block;
    intptr_t block_size;
    for (block = heap_start; block < heap_end; block += block_size) {
        block_size = YA_SZ_BLK(block);
        printf("Block at %p  size = %ld  alloc = %d\n",
                block, block_size, YA_IS_ALLOC_BLK(block));
    }
}
#endif

void block_init(intptr_t *block, intptr_t size) {
    block[-1]       = size;
    block[size - 2] = size;
}

void block_alloc(intptr_t *block) {
    intptr_t block_size = YA_SZ_BLK(block);
    block[-1]           |= 1;
    block[block_size-2] |= 1;
}

void block_free(intptr_t *block) {
    intptr_t block_size = YA_SZ_BLK(block);
    block[-1]           &= -2;
    block[block_size-2] &= -2;
}

intptr_t *block_join_prev(intptr_t *block) {
    intptr_t prev_size = YA_SZ_TAG(block[-2]);
    intptr_t *prev = block - prev_size;
    if (prev <= heap_start || YA_IS_ALLOC_BLK(prev)) {
        return block;
    }
    intptr_t block_size = YA_SZ_BLK(block);
    block_init(prev, prev_size + block_size);
    ya_debug("block_join_prev: joining %p:%ld and %p:%ld -> %p:%ld\n",
            block, block_size, prev, prev_size, prev, prev_size + block_size);
    return prev;
}

intptr_t *block_join_next(intptr_t *block) {
    intptr_t block_size = YA_SZ_BLK(block);
    intptr_t *next = block + block_size;
    if (next >= heap_end || YA_IS_ALLOC_BLK(next)) {
        return block;
    }
    intptr_t next_size = YA_SZ_BLK(next);
    block_init(block, block_size + next_size);
    ya_debug("block_join_next: joining %p:%ld and %p:%ld -> %p:%ld\n",
            block, block_size, next, next_size, block, block_size + next_size);
    return block;
}

intptr_t *block_join(intptr_t *block) {
    if (block > heap_start) {
        block = block_join_prev(block);
    }
    return block_join_next(block);
}

void block_split(intptr_t *block, intptr_t size) {
    block_init(block + size, YA_SZ_BLK(block) - size);
    block_init(block, size);
}

intptr_t *block_find(intptr_t size) {
    intptr_t *block;
    intptr_t block_size;
    for (block = heap_start; block < heap_end; block += block_size) {
        block_size = YA_SZ_BLK(block);
        if (!YA_IS_ALLOC_BLK(block) && size <= block_size) {
            return block;
        }
    }
    // could not find block, extend heap
    return heap_extend(size);
}

intptr_t *heap_init() {
    intptr_t size_w = YA_SZ_CHUNK / YA_SZ_WORD;
    void *ptr = sbrk(YA_SZ_WORD * (size_w + 2));
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

intptr_t *heap_extend(intptr_t size_w) {
    intptr_t size = YA_ROUND(size_w * YA_SZ_WORD, YA_SZ_CHUNK);
    size_w = size / YA_SZ_WORD;
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
    size_w = 2 + YA_ROUND(size_w, 2); // round to dword and make space for tags
    ya_debug("malloc: requested = %ld, allocating = %ld * %ld = %ld\n",
            size, size_w, YA_SZ_WORD, size_w * YA_SZ_WORD);
    intptr_t *block = block_find(size_w);
    intptr_t block_size = YA_SZ_BLK(block);
    if (size_w < block_size) {
        block_split(block, size_w);
    }
    block_alloc(block);
    return block;
}

void free(void *ptr) {
    intptr_t *block = ptr;
    if (block < heap_start || block > heap_end || !YA_IS_ALLOC_BLK(block)) {
        return; // TODO: provoke segfault
    }
    block_free(block);
    block_join(block);
}
