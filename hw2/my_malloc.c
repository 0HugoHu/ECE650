//
// Created by linke li on 1/24/23.
//

#include "my_malloc.h"



size_t data_space = 0;
// 维护一个空闲链表;头节点
//block *linked_list = NULL;

block *first_free_lock = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
__thread block *first_free_nolock = NULL;


//Thread Safe malloc/free: locking version
void *ts_malloc_lock(size_t size) {
    pthread_mutex_lock(&lock);
    int sbrk_lock = 0;
    void *p = bf_malloc(size, first_free_lock, sbrk_lock);
    pthread_mutex_unlock(&lock);
    return p;
}

void ts_free_lock(void *ptr) {
    pthread_mutex_lock(&lock);
    bf_free(ptr, first_free_lock);
    pthread_mutex_unlock(&lock);
}

//Thread Safe malloc/free: non-locking version
void *ts_malloc_nolock(size_t size) {
    int sbrk_lock = 1;
    void *p = bf_malloc(size, first_free_nolock, sbrk_lock);
    return p;
}

void ts_free_nolock(void *ptr) {
    bf_free(ptr, first_free_lock);
}



void *bf_malloc(size_t size, block * linked_list, int sbrk_lock) {
    block *cur = linked_list;
//    block *start = linked_list; // for reference
    int cur_is_head = 0;
    block *best_fit_ptr = NULL;
    // step1:寻找合适的内存块

    while (cur) {
        if (cur->bsize >= size + sizeof(block)) { // 如果当前内存大小>=所需大小
            if (best_fit_ptr == NULL) {
                best_fit_ptr = cur;
            } else if (best_fit_ptr->bsize > cur->bsize) {
                best_fit_ptr = cur;
            }
            if (cur->bsize == size + sizeof(block)) break;
        }
        cur = cur->next; // 内存大小不满足，继续寻找下一个内存块
    }

    // 找到后从链表移除
    if (best_fit_ptr != NULL) {
        //printf("FOUND%lu\n", best_fit_ptr->bsize);
        if (best_fit_ptr->pre) { // 如果不是头节点
            best_fit_ptr->pre->next = best_fit_ptr->next;
        } else { // 如果是头节点
            cur_is_head = 1;
        }
        if (best_fit_ptr->next) { // 如果不是尾节点
            best_fit_ptr->next->pre = best_fit_ptr->pre;
        }
    }

    cur = best_fit_ptr;
    // 如果没找到，开辟新的内存空间
    if (cur == NULL) {
        block *new_block = NULL;
        //printf("MAKE NEW\n");
        if (sbrk_lock == 0) {
            new_block = (block *) sbrk(size + sizeof(block)); // 分配所需大小+metadata大小
        } else {
            pthread_mutex_lock(&lock);
            new_block = (block *) sbrk(size + sizeof(block));
            pthread_mutex_unlock(&lock);
        }

        if (new_block == (void *) -1) {                // 判断指针是否返回正确
            return NULL; // 如果失败,直接退出函数，避免further problems
        }
        // clarify property of new block
        new_block->bsize = size + sizeof(block);
        new_block->pre = NULL;
        new_block->next = NULL;
        cur = new_block;
        data_space += cur->bsize;
    }
    size_t tmp_bsize = cur->bsize;
    int cur_need_split = 0;
    block *splitted_block = NULL;
    // step2: split memory space
    if (cur->bsize > size + 2 * sizeof(block)) {
        //printf("SPLIT\n");
        cur_need_split = 1;
        size_t nex = (size_t)((char *) cur + tmp_bsize);
        splitted_block = (block *) ((char *) cur + size + sizeof(block)); // the empty block
        // cur -------移除链表
        splitted_block->next = cur->next;
        splitted_block->pre = cur->pre;
        if (splitted_block->pre) {
            cur->pre->next = splitted_block;
        }
        if (splitted_block->next) {
            splitted_block->next->pre = splitted_block;
        }
        // property
        cur->bsize = size + sizeof(block);
        splitted_block->bsize = tmp_bsize - size - sizeof(block);

        if (!linked_list) {
            linked_list = splitted_block;
        } // 如果此时链表为空，加入temp0为头节点
    }

    if (cur_is_head && cur_need_split) {
        linked_list = splitted_block;
    } else if (cur_is_head && !cur_need_split) {
        linked_list = linked_list->next;
    }

    cur->pre = NULL;
    cur->next = NULL;
    return (char *) cur + sizeof(block);
}

void bf_free(void *ptr, block * linked_list) {
//    block *start = linked_list;
    block *var = linked_list;
    block *var_pre = linked_list;
    if (ptr == NULL) {
        return;
    }
    // step1:获取该block
    block *cur = ptr - sizeof(block); // 找到该地址对应的要释放的block
    if (!var) {
        linked_list = cur;
    } // 如果是空链表，表头即为cur
    //  step2：找到其相应链表的位置并加入
    while (var) {
        if (var > cur) { // 找到相应地址
            if (var->pre) { // var->pre-----cur------var
                cur->pre = var->pre;
                var->pre->next = cur;
            } else { //(head)cur------var
                linked_list = cur;
            }
            cur->next = var;
            var->pre = cur;
            break;
        }
        var_pre = var;
        var = var->next; // 地址不满足
    }
    if (!var && var_pre) {
        var_pre->next = cur;
        cur->pre = var_pre;
    }
    // step3:合并
    block *next_block = cur;
    block *pre_block = cur;
    size_t cur_size = cur->bsize;
    //-------------合并下一个空闲的内存空间
    while (next_block->next) {
        if ((size_t) next_block->next != (size_t) next_block + next_block->bsize) {
            break;
        }                              // 如果相邻的非空闲，不再判断
        cur_size += next_block->next->bsize; // 问meta data要算进去吗？这里的是算进去的版本
        next_block = next_block->next;
    }
    // 合并之前的空闲内存空间
    while (pre_block->pre) {
        if ((size_t) pre_block->pre + pre_block->pre->bsize != (size_t) pre_block) {
            break;
        }                             // 如果相邻的非空闲，不再判断
        cur_size += pre_block->pre->bsize; // 问meta data要算进去吗？这里的是算进去的版本
        pre_block = pre_block->pre;
        // pre_block->next=NULL;
    }
    // 合并为新的内存块
    // pointer关系
    // 如果有可合并的
    if (next_block != cur || pre_block != cur) {
        // printf("merge\n");
        pre_block->bsize = cur_size;
        pre_block->next = next_block->next;
        if (next_block->next) {
            next_block->next->pre = pre_block;
        }
    }
}


