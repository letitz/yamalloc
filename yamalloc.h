/*
 * Yet Another Malloc
 * yamalloc.h
 */

#ifndef YAMALLOC_H
#define YAMALLOC_H

#include <stddef.h> // for size_t

void *malloc(size_t size);

void free(void *ptr);

void *calloc(size_t nmemb, size_t size);

void *realloc(void *ptr, size_t size);

#endif // def YAMALLOC_H
