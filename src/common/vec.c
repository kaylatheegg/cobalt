#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "vec.h"
#include "crash.h"

_VecGeneric* _vec_new(size_t stride, size_t initial_cap) {
    // store the vec statically so it lives after vec_new has been called, 
    // long enough for it to be copied out on the caller side
    _Thread_local static _VecGeneric temp;
    _vec_init(&temp, stride, initial_cap);
    return &temp;
}

void _vec_init(_VecGeneric* v, size_t stride, size_t initial_cap) {
    v->at = malloc(stride * initial_cap);
    v->cap = initial_cap;
    v->len = 0;
}

void _vec_reserve(_VecGeneric* v, size_t stride, size_t slots) {
    if (v->len > v->cap) crash("%s: v->len > v->cap\n", __func__);
    if (slots + v->len > v->cap) {
        v->cap += slots;
        v->at = realloc(v->at, v->cap * stride);
    }
}

void _vec_expand_if_necessary(_VecGeneric* v, size_t stride) {
    if (v->len > v->cap) crash("%s: v->len > v->cap\n", __func__);

    if (v->len + 1 > v->cap) {
        if (v->cap == 1) v->cap++;
        v->cap = (v->cap * 3) / 2;
        v->at = realloc(v->at, v->cap * stride);
    }
}

void _vec_destroy(_VecGeneric* v) {
    if (v->len > v->cap) crash("%s: v->len > v->cap\n", __func__);
    
    free(v->at);
    *v = (_VecGeneric){0};
}

void _vec_shrink(_VecGeneric* v, size_t stride) {
    if (v->len > v->cap) crash("%s: v->len > v->cap\n", __func__);
    
    if (v->len == v->cap) {
        return;
    }
    v->at = realloc(v->at, v->len * stride);
    v->cap = v->len;
}

void _vec_remove(_VecGeneric* v, size_t stride, size_t index) {
    if (v->len > v->cap) crash("%s: v->len > v->cap\n", __func__);
    
    if (v->len < index) return;

    //we move everything above down
    memmove(v->at + index * stride, v->at + (index + 1) * stride, stride * (v->len - index - 1));
    v->len--;
}

void _vec_insert_space(_VecGeneric* v, size_t stride, size_t index) {
    if (v->len > v->cap) crash("%s: v->len > v->cap\n", __func__);
    
    if (v->len < index) return;
    //we expand if needed
    _vec_expand_if_necessary(v, stride);
    v->len++;
    //then, we insert a space at the index, after moving all elements up by one
    memmove(v->at + (index + 1) * stride, v->at + index * stride, stride * (v->len - index - 1));
}