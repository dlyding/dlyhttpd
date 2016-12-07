#ifndef __HEAP_H__
#define __HEAP_H__

#include "dbg.h"
#include "util.h"

#define HEAP_DEFAULT_SIZE 8

typedef int (*heap_comp_function_ptr)(void *pi, void *pj);

typedef struct heap_s{
    void **contptr;                 // 存放堆数据的指针，下标从零开始
    size_t size;                    // 此时堆的实际数据大小
    size_t cap;                     // 容量
    heap_comp_function_ptr comp;    // 比较函数
} heap_t;

heap_t* heap_create(heap_comp_function_ptr comp, size_t cap);
int heap_is_empty(heap_t *ht);
size_t heap_size(heap_t *ht);
void *heap_top(heap_t *ht);
int heap_deltop(heap_t *ht);
int heap_insert(heap_t *ht, void *item);
int heap_close(heap_t* ht);

#endif 
