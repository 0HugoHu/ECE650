//
// Created by linke li on 1/24/23.
//

#include "my_malloc.h"

// Define memory block
struct mem_block
{
    size_t bsize;           // block size (include meta data)
    struct mem_block *next; // 下一个相邻内存块地址
    struct mem_block *pre;  // 上一个相邻内存块地址
} mem_block;
typedef struct mem_block block;

size_t MAX_FREE = 0;

size_t free_space = 0;

size_t data_space = 0;

// 维护一个空闲链表;头节点
block *linked_list = NULL;

// Tail
block *linked_list_tail = NULL;

void *ff_malloc(size_t size)
{

    block *cur = linked_list;
    block *cur_pre = linked_list;
    block *start = linked_list; // for reference
    int tag = 0;

    // step1:寻找合适的内存块

    while (cur)
    {
        if (cur->bsize >= size + sizeof(block))
        { // 如果当前内存大小>=所需大小

            // 找到后从链表移除
            if (cur->pre)
            { // 如果不是头节点
                cur->pre->next = cur->next;
            }
            else
            { // 如果是头节点
                tag = 1;
            }
            if (cur->next)
            { // 如果不是尾节点
                cur->next->pre = cur->pre;
            }
            break;
        }

        cur_pre = cur;   // 记录上一个block
        cur = cur->next; // 内存大小不满足，继续寻找下一个内存块
    }

    int new_space = 0;
    // 如果没找到，开辟新的内存空间
    if (cur == NULL)
    {
        new_space = 1;
        // printf("MAKE NEW\n");
        block *new = sbrk(0);
        new = (block *)sbrk(size + sizeof(block)); // 分配所需大小+metadata大小
        if (new == (void *)-1)
        {                // 判断指针是否返回正确
            return NULL; // 如果失败,直接退出函数，避免further problems
        }

        // clarify property of new block
        new->bsize = size + sizeof(block);

        // new->tag=1;
        new->pre = NULL;
        new->next = NULL;
        cur = new;
        data_space += cur->bsize;
    }

    size_t tmp_bsize = cur->bsize;

    int tag2 = 0;
    block *temp0 = NULL;

    // step2: split memory space
    if (cur->bsize > size + 2 * sizeof(block))
    {
        tag2 = 1;
        size_t nex = (size_t)((char *)cur + tmp_bsize);
        temp0 = (block *)((char *)cur + size + sizeof(block)); // the empty block

        // cur -------移除链表
        temp0->next = cur->next;
        temp0->pre = cur->pre;
        if (temp0->pre)
        {
            cur->pre->next = temp0;
        }
        if (temp0->next)
        {
            temp0->next->pre = temp0;
        }

        // property
        cur->bsize = size + sizeof(block);

        // cur->tag=1;   //occupied space
        temp0->bsize = tmp_bsize - size - sizeof(block);

        if (!linked_list)
        {
            linked_list = temp0;
        } // 如果此时链表为空，加入temp0为头节点
        // if (!temp0->pre){linked_list=temp0;}

        //        if (cur->pre==NULL){linked_list=temp0;} //如果为头节点
    } 

    if (!new_space) {
        free_space -= cur->bsize;
    }

    if (tag && tag2)
    {
        linked_list = temp0;
    }
    else if (tag && !tag2)
    {
        linked_list = linked_list->next;
    }

    cur->pre = NULL;
    cur->next = NULL;
    // print_list();
    return (char *)cur + sizeof(block);
}

// free memory allocated
void ff_free(void *ptr)
{
    block *start = linked_list;
    block *var = linked_list;
    block *var_pre = linked_list;
    if (ptr == NULL)
    {
        return;
    }
    // step1:获取该block
    block *cur = ptr - sizeof(block); // 找到该地址对应的要释放的block
    free_space += cur->bsize;

    if (!var)
    {
        linked_list = cur;
    } // 如果是空链表，表头即为cur

    //  step2：找到其相应链表的位置并加入
    while (var)
    {
        if (var > cur)
        { // 找到相应地址
            if (var->pre)
            { // var->pre-----cur------var
                cur->pre = var->pre;
                var->pre->next = cur;
                // print_list();
            }
            else
            { //(head)cur------var
                linked_list = cur;
            }
            cur->next = var;
            var->pre = cur;
            // print_list();
            break;
        }
        var_pre = var;
        var = var->next; // 地址不满足
    }
    if (!var && var_pre)
    {
        var_pre->next = cur;
        cur->pre = var_pre;
    }

    // step3:合并
    block *tmp0 = cur;
    block *tmp1 = cur;
    size_t cur_size = cur->bsize;
    //-------------合并下一个空闲的内存空间
    while (tmp0->next)
    {
        if ((size_t)tmp0->next != (size_t)tmp0 + tmp0->bsize)
        {
            break;
        }                              // 如果相邻的非空闲，不再判断
        cur_size += tmp0->next->bsize; // 问meta data要算进去吗？这里的是算进去的版本
        tmp0 = tmp0->next;
    }
    // 合并之前的空闲内存空间
    while (tmp1->pre)
    {
        if ((size_t)tmp1->pre + tmp1->pre->bsize != (size_t)tmp1)
        {
            break;
        }                             // 如果相邻的非空闲，不再判断
        cur_size += tmp1->pre->bsize; // 问meta data要算进去吗？这里的是算进去的版本
        tmp1 = tmp1->pre;
        // tmp1->next=NULL;
    }
    // 合并为新的内存块
    // pointer关系
    // 如果有可合并的
    if (tmp0 != cur || tmp1 != cur)
    {
        // printf("merge\n");
        // print_list();
        tmp1->bsize = cur_size;
        tmp1->next = tmp0->next;

        if (tmp0->next)
        {
            tmp0->next->pre = tmp1;
        }
    }
}

void print_list()
{
    block *b = linked_list;
    int i = 0;
    while (b != NULL && i < 100)
    {
        // printf("%zu->", (size_t)b);
        if (b == b->next)
        {
            exit(EXIT_FAILURE);
        }
        b = b->next;
        i++;
    }
}
void *bf_malloc(size_t size)
{
    block *cur = linked_list;
    block *start = linked_list; // for reference
    int tag = 0;

    block *best_fit_ptr = NULL;

    // step1:寻找合适的内存块

    while (cur)
    {
        if (cur->bsize >= size + sizeof(block))
        { // 如果当前内存大小>=所需大小
            if (best_fit_ptr == NULL)
            {
                best_fit_ptr = cur;
            }
            else if (best_fit_ptr->bsize > cur->bsize)
            {
                best_fit_ptr = cur;
            }
            if (cur->bsize == size + sizeof(block)) break;
        }
        cur = cur->next; // 内存大小不满足，继续寻找下一个内存块
    }

    // 找到后从链表移除
    if (best_fit_ptr != NULL)
    {
        //printf("FOUND%lu\n", best_fit_ptr->bsize);
        if (best_fit_ptr->pre)
        { // 如果不是头节点
            best_fit_ptr->pre->next = best_fit_ptr->next;
        }
        else
        { // 如果是头节点
            tag = 1;
        }
        if (best_fit_ptr->next)
        { // 如果不是尾节点
            best_fit_ptr->next->pre = best_fit_ptr->pre;
        }
    }

    cur = best_fit_ptr;

    int new_space = 0;

    // 如果没找到，开辟新的内存空间
    if (cur == NULL)
    {
        new_space = 1;
        //printf("MAKE NEW\n");
        block *new = sbrk(0);
        new = (block *)sbrk(size + sizeof(block)); // 分配所需大小+metadata大小
        if (new == (void *)-1)
        {                // 判断指针是否返回正确
            return NULL; // 如果失败,直接退出函数，避免further problems
        }

        // clarify property of new block
        new->bsize = size + sizeof(block);

        // new->tag=1;
        new->pre = NULL;
        new->next = NULL;
        cur = new;
        data_space += cur->bsize;
    }

    size_t tmp_bsize = cur->bsize;

    int tag2 = 0;
    block *temp0 = NULL;
    // step2: split memory space
    if (cur->bsize > size + 2 * sizeof(block))
    {
        //printf("SPLIT\n");
        tag2 = 1;
        size_t nex = (size_t)((char *)cur + tmp_bsize);
        temp0 = (block *)((char *)cur + size + sizeof(block)); // the empty block

        // cur -------移除链表
        temp0->next = cur->next;
        temp0->pre = cur->pre;
        if (temp0->pre)
        {
            cur->pre->next = temp0;
        }
        if (temp0->next)
        {
            temp0->next->pre = temp0;
        }

        // property
        cur->bsize = size + sizeof(block);

        // cur->tag=1;   //occupied space
        temp0->bsize = tmp_bsize - size - sizeof(block);

        if (!linked_list)
        {
            linked_list = temp0;
            linked_list_tail = temp0;
        } // 如果此时链表为空，加入temp0为头节点
        // if (!temp0->pre){linked_list=temp0;}

        //        if (cur->pre==NULL){linked_list=temp0;} //如果为头节点
    }

    if (!new_space) {
        free_space -= cur->bsize;
    }

    if (tag && tag2)
    {
        linked_list = temp0;
    }
    else if (tag && !tag2)
    {
        linked_list = linked_list->next;
    }

    cur->pre = NULL;
    cur->next = NULL;
    // print_list();
    return (char *)cur + sizeof(block);
}

void bf_free(void *ptr)
{
    block *start = linked_list;
    block *var = linked_list;
    block *var_pre = linked_list;
    if (ptr == NULL)
    {
        return;
    }
    // step1:获取该block
    block *cur = ptr - sizeof(block); // 找到该地址对应的要释放的block

    free_space += cur->bsize;

    if (!var)
    {
        linked_list = cur;
    } // 如果是空链表，表头即为cur

    //  step2：找到其相应链表的位置并加入
    while (var)
    {
        if (var > cur)
        { // 找到相应地址
            if (var->pre)
            { // var->pre-----cur------var
                cur->pre = var->pre;
                var->pre->next = cur;
                // print_list();
            }
            else
            { //(head)cur------var
                linked_list = cur;
            }
            cur->next = var;
            var->pre = cur;
            // print_list();
            break;
        }
        var_pre = var;
        var = var->next; // 地址不满足
    }
    if (!var && var_pre)
    {
        var_pre->next = cur;
        cur->pre = var_pre;
    }

    // step3:合并
    block *tmp0 = cur;
    block *tmp1 = cur;
    size_t cur_size = cur->bsize;
    //-------------合并下一个空闲的内存空间
    while (tmp0->next)
    {
        if ((size_t)tmp0->next != (size_t)tmp0 + tmp0->bsize)
        {
            break;
        }                              // 如果相邻的非空闲，不再判断
        cur_size += tmp0->next->bsize; // 问meta data要算进去吗？这里的是算进去的版本
        tmp0 = tmp0->next;
    }
    // 合并之前的空闲内存空间
    while (tmp1->pre)
    {
        if ((size_t)tmp1->pre + tmp1->pre->bsize != (size_t)tmp1)
        {
            break;
        }                             // 如果相邻的非空闲，不再判断
        cur_size += tmp1->pre->bsize; // 问meta data要算进去吗？这里的是算进去的版本
        tmp1 = tmp1->pre;
        // tmp1->next=NULL;
    }
    // 合并为新的内存块
    // pointer关系
    // 如果有可合并的
    if (tmp0 != cur || tmp1 != cur)
    {
        // printf("merge\n");
        // print_list();
        tmp1->bsize = cur_size;
        tmp1->next = tmp0->next;

        if (tmp0->next)
        {
            tmp0->next->pre = tmp1;
        }
    }
}

// entire heap memory
unsigned long get_data_segment_size()
{
    // char *cur = sbrk(0);
    // return cur - (char *)linked_list;
    return data_space;
};

// get usable free space + space occupied by corresponding metadata
unsigned long get_data_segment_free_space_size()
{
    // block *cur = linked_list;
    // unsigned long free_space_size = 0;
    // while (cur != NULL)
    // {
    //     //        if (cur->tag==0){
    //     free_space_size += cur->bsize;
    //     //        }
    //     cur = cur->next;
    // }
    // return free_space_size;
    return free_space;
};
