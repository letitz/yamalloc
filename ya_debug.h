/*
 * Yet Another Malloc
 * ya_debug.h
 */

#ifndef YA_DEBUG_H
#define YA_DEBUG_H

#include <stdio.h> // for fprintf, so that other files may simply include this

#ifdef YA_DEBUG // enables debugging

#define ya_debug(...) fprintf(stderr, __VA_ARGS__)

void ya_print_blocks();

/* Checks internal state for errors.
 * Returns -1 on error, 0 otherwise. */
int ya_check();

#else

#define ya_debug(...)
#define ya_print_blocks(...)
#define ya_check(...)

#endif // def YA_DEBUG

#endif // ndef YADEBUG_H
