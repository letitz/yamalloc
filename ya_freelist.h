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

/*------------*/
/* Data types */
/*------------*/

struct fl {
    intptr_t *block;
    struct fl *prev;
    struct fl *next;
};

/*--------------*/
/* Declarations */
/*--------------*/

struct fl *fl_find(intptr_t min_size);

void fl_split(struct fl *fl);

void fl_join_prev(struct fl *fl);

void fl_join_next(struct fl *fl);

void fl_join(struct fl *fl);

#endif
