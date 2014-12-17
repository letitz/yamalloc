/*
 * Yet Another Malloc
 * yamalloc.h
 */

#ifndef YAMALLOC_H
#define YAMALLOC_H

#include <stddef.h>

#ifdef YA_DEBUG

#define ya_debug(...) fprintf(stderr, __VA_ARGS__)

void ya_print_blocks();

#else

#define ya_debug(...)
#define ya_print_blocks(...)

#endif

void *malloc(size_t size);

void free(void *ptr);

#endif
