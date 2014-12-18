/*
 * Yet Another Malloc
 * yatest.c
 * Author: Titouan Rigoudy
*/

#include <stdio.h>

#include "yamalloc.h"

void *print_malloc(size_t size) {
    void *ptr = malloc(size);
    fprintf(stderr, "malloc(%ld) = %p\n", size, ptr);
    return ptr;
}

void print_free(void *ptr) {
    fprintf(stderr, "free(%p)\n", ptr);
    free(ptr);
}

void *print_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    fprintf(stderr, "realloc(%p, %ld) = %p\n", ptr, size, new_ptr);
    return new_ptr;
}

int main(int argc, char **argv) {
    void *a, *b, *c, *d;
    ya_print_blocks();
    a = print_malloc(4);
    ya_print_blocks();
    b = print_malloc(10);
    ya_print_blocks();
    c = print_malloc(10000);
    ya_print_blocks();
    d = print_malloc(2000);
    ya_print_blocks();
    print_free(a);
    ya_print_blocks();
    print_free(c);
    ya_print_blocks();
    c = print_malloc(100);
    ya_print_blocks();
    a = print_malloc(2);
    ya_print_blocks();
    print_free(d);
    ya_print_blocks();
    d = print_realloc(NULL, 400);
    ya_print_blocks();
    d = print_realloc(d, 4000);
    ya_print_blocks();
    d = print_realloc(d, 32000);
    ya_print_blocks();
    d = print_realloc(d, 1000);
    ya_print_blocks();
}
