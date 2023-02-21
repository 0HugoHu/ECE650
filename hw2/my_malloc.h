//
// Created by linke li on 1/19/23.
//
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#ifndef INC_651_1_MY_MALLOC_H
#define INC_651_1_MY_MALLOC_H

// Define memory block
struct mem_block {
    size_t bsize;           // block size (include meta data)
    struct mem_block *next; // 下一个相邻内存块地址
    struct mem_block *pre;  // 上一个相邻内存块地址
} mem_block;
typedef struct mem_block block;

void print_list();

void *bf_malloc(size_t size, block * linked_list, int sbrk_lock);

void bf_free(void *ptr, block * linked_list);


//Thread Safe malloc/free: locking version
void *ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr);

//Thread Safe malloc/free: non-locking version
void *ts_malloc_nolock(size_t size);
void ts_free_nolock(void *ptr);

#endif //INC_651_1_MY_MALLOC_H
