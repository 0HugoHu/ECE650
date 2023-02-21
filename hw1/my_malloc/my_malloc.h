//
// Created by linke li on 1/19/23.
//
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#ifndef INC_651_1_MY_MALLOC_H
#define INC_651_1_MY_MALLOC_H

struct blcok;
void print_list();
void * ff_malloc(size_t size);

void ff_free(void *ptr);

void *bf_malloc(size_t size);

void bf_free(void *ptr);

unsigned long get_data_segment_size();

unsigned long get_data_segment_free_space_size();

#endif //INC_651_1_MY_MALLOC_H
