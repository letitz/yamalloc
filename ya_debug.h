/*
 * Yet Another Malloc
 * ya_debug.h
 */

#ifndef YA_DEBUG_H
#define YA_DEBUG_H

#ifdef YA_DEBUG // enables debugging

#define ya_debug(...) fprintf(stderr, __VA_ARGS__)

void ya_print_blocks();

#else

#define ya_debug(...)
#define ya_print_blocks(...)

#endif // def YA_DEBUG

#endif // ndef YADEBUG_H
