#include "heap.h"

static int resize(heap_t *ht, size_t new_cap);
static void swap(heap_t *ht, size_t i, size_t j);
static void swim(heap_t *ht, size_t k);
static size_t sink(heap_t *ht, size_t k);

static int resize(heap_t *ht, size_t new_cap) {
    if (new_cap <= ht->size) {
        log_err("resize: new_size to small");
        return -1;
    }

    void **new_ptr = (void **)malloc(sizeof(void *) * new_cap);
    if (!new_ptr) {
        log_err("resize: malloc failed");
        return -1;
    }

    memcpy(new_ptr, ht->contptr, sizeof(void *) * (ht->size));
    free(ht->contptr);
    ht->contptr = new_ptr;
    ht->cap = new_cap;
    return DLY_OK;
}

static void swap(heap_t *ht, size_t i, size_t j) {
    if(i == j) 
        return;
    void *tmp = ht->contptr[i];
    ht->contptr[i] = ht->contptr[j];
    ht->contptr[j] = tmp;
}

// k从零开始
static void swim(heap_t *ht, size_t k) {
    while (k > 0) {
        if(ht->comp(ht->contptr[k], ht->contptr[(k-1)/2])) {
            swap(ht, k, (k-1)/2);
            k = (k-1)/2;
        }
        else
            break;
    }
}

// k从零开始
static size_t sink(heap_t *ht, size_t k) {
    size_t j;
    size_t endi = ht->size - 1;

    while (2 * k + 1 <= endi) {
        j = 2 * k + 1;
        if (j < endi && ht->comp(ht->contptr[j+1], ht->contptr[j])) 
            j++;
        if (!ht->comp(ht->contptr[j], ht->contptr[k])) 
            break;
        swap(ht, j, k);
        k = j;
    }
    
    return k;
}


heap_t* heap_create(heap_comp_function_ptr comp, size_t cap) {
    heap_t* ht = (heap_t*)malloc(sizeof(heap_t));
    if(ht == NULL) {
        log_err("ht_init: ht malloc failed");
        return NULL;
    }      
    ht->contptr = (void **)malloc(sizeof(void *) * (cap));
    if (!ht->contptr) {
        log_err("ht_init: malloc failed");
        return NULL;
    }
    
    ht->size = 0;
    ht->cap = cap;
    ht->comp = comp;
    
    return ht;
}

int heap_is_empty(heap_t *ht) {
    return (ht->size == 0)? 1: 0;
}

size_t heap_size(heap_t *ht) {
    return ht->size;
}

void *heap_top(heap_t *ht) {
    if (heap_is_empty(ht)) {
        return NULL;
    }

    return ht->contptr[0];
}

int heap_deltop(heap_t *ht) {
    if (heap_is_empty(ht)) {
        return DLY_OK;
    }
    swap(ht, 0, ht->size - 1);
    ht->size--;
    // 不判断会导致进程退出
    if(ht->size > 0)
        sink(ht, 0);
    if (ht->size > 0 && ht->size <= (ht->cap - 1)/4) {
        if (resize(ht, ht->cap / 2) < 0) {
            return -1;
        }
    }

    return DLY_OK;
}

int heap_insert(heap_t *ht, void *item) {
    if (ht->size == ht->cap) {
        if (resize(ht, ht->cap * 2) < 0) {
            return -1;
        }
    }

    ht->contptr[ht->size++] = item;
    swim(ht, ht->size-1);

    return DLY_OK;
}

/*int heap_sink(heap_t *ht, size_t i) {
    return sink(ht, i);
}*/

int heap_close(heap_t* ht) {
    /*for(int i = 0; i < ht->size; i++) {
        free(ht->contptr[i]);
    }*/
    if(ht->size > 0) {
        return -1;
    }
    free(ht);
    return DLY_OK;
}