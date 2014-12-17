/*
 * Yet Another Malloc
 * yatest.c
 * Author: Titouan Rigoudy
*/

#include <stdio.h>

#include "yamalloc.h"

void *print_malloc(size_t size) {
    void *ptr = malloc(size);
    printf("malloc(%ld) = %p\n", size, ptr);
    return ptr;
}

int main(int argc, char **argv) {
    ya_print_blocks();
    print_malloc(4);
    print_malloc(10);
    ya_print_blocks();
    print_malloc(10000);
    ya_print_blocks();
    print_malloc(2000);
    ya_print_blocks();
}
