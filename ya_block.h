/*
 * Yet Another Malloc
 * ya_block.h
 */

#ifndef YA_BLOCK_H
#define YA_BLOCK_H

/*----------*/
/* Includes */
/*----------*/

#include <stddef.h> // for size_t
#include <stdint.h> // for intptr_t

/*-----------*/
/* Constants */
/*-----------*/

#define YA_SZ_WORD  (sizeof(intptr_t)) // big enough to hold a pointer
#define YA_SZ_DWORD (2 * YA_SZ_WORD)   // storage is aligned to a dword
#define YA_SZ_CHUNK 8192               // request memory 8k by 8k from OS

#define YA_MIN_SZ_BLK 4 // smallest block: dword-aligned with two boundary tags

/*--------*/
/* Macros */
/*--------*/

#define YA_IS_ALLOC_TAG(tag) ((tag) & 1)
#define YA_SZ_TAG(tag)       ((tag) & -2)

#define YA_IS_ALLOC_BLK(block) YA_IS_ALLOC_TAG((block)[-1])
#define YA_SZ_BLK(block)       YA_SZ_TAG((block)[-1])

#define YA_IS_ALLOC_END(b_end) YA_IS_ALLOC_TAG((b_end)[0])
#define YA_SZ_END(b_end)       YA_SZ_TAG((b_end)[0])

#define YA_ROUND_DIV(n, m) (((n) + ((m)-1)) / (m))
#define YA_ROUND(n, m)     (YA_ROUND_DIV(n,m) * (m))

/*---------*/
/* Globals */
/*---------*/

extern intptr_t *heap_start; // with space for 2 words before
extern intptr_t *heap_end  ; // first block outside heap

/*--------------*/
/* Declarations */
/*--------------*/

/* Initializes the heap by calling sbrk to allocate some starter memory.
 * Sets heap_start and heap_end to their appropriate values.
 * Returns the pointer to the start of the heap or NULL in case of failure. */
intptr_t *heap_init();

/* Extends the heap by at least size_w words by calling sbrk.
 * Returns a pointer to the last (free) block or NULL in case of failure. */
intptr_t *heap_extend(intptr_t size_w);

#ifdef YA_DEBUG
/* Prints each block in the range from the block at start to the one at end */
void block_print_range(intptr_t *start, intptr_t *end);
#endif

/* Initializes the block's boundary tags. */
void block_init(intptr_t *block, intptr_t size);

/* Sets the allocated bit in the block's boundary tags. */
void block_alloc(intptr_t *block);

/* Erases the allocated bit in the block's boundary tags. */
void block_free(intptr_t *block);

/* Returns the size in words of the smallest block that can
 * store n_bytes bytes. Takes alignment and boundary tags into account */
intptr_t block_fit(size_t n_bytes);

/* Tries to coalesce a block with its previous neighbor.
 * Returns a pointer to the coalesced block. */
intptr_t *block_join_prev(intptr_t *block);

/* Tries to colesce a block with its next neighbor.
 * Returns the unchanged pointer to the block. */
intptr_t *block_join_next(intptr_t *block);

/* Tries to coalesce a block with its previous and next neighbors.
 * Returns a pointer to the coalesced block. */
intptr_t *block_join(intptr_t *block);

/* Split the block [block_size] into [size, block_size - size] if possible
 * Returns a pointer to the second block or NULL if no split occurred. */
intptr_t *block_split(intptr_t *block, intptr_t size);

/* Try to find a free block at least size words big by walking the boundary
 * tags. If no block is found the heap is grown adequately.
 * Returns a pointer to the block or NULL in case of failure. */
intptr_t *block_find(intptr_t size);

#endif
