#ifndef VECTOR_H
#define VECTOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    void *p;
    size_t size;
    size_t length;
    size_t capacity;
} vector;

vector* vector_new(size_t size);
void vector_free(vector **v);
bool vector_grow(vector *v, size_t length);
size_t vector_push(vector *v, const void *val);
size_t vector_push_i(vector *v, const void *val, size_t *i);
size_t vector_push_n(vector *v, const void *val, size_t count);
bool vector_pop(vector *v, void *dst);
bool vector_clear(vector *v);
bool vector_contains(vector *v, const void *val);
bool vector_push_unique(vector *v, const void *val);
bool vector_push_unique_i(vector *v, const void *val, size_t *i);
bool vector_find(vector *v, const void *val, size_t *dst);
#endif

#ifdef VECTOR_IMPLEMENTATION

#include "macro.h"

#define VECTOR_INITIAL_CAPACITY 128

vector* vector_new(size_t size) {
    if(!size) return NULL;
    vector *ret = malloc(sizeof(vector));
    if(!ret) return NULL;
    ret->p = malloc(size * VECTOR_INITIAL_CAPACITY);
    if(!ret->p) return NULL;
    ret->size = size;
    ret->length = 0;
    ret->capacity = VECTOR_INITIAL_CAPACITY;
    return ret;
}

void vector_free(vector **v) {
    if(*v) {
        if((*v)->p) {
            free((*v)->p);
            (*v)->p = NULL;
        }
        free(*v);
        *v = NULL;
    }
}

bool vector_grow(vector *v, size_t length) {
    if(length < v->capacity) return true;
    size_t new_capacity = MAX(v->capacity * 2, length);
    void *new_p = realloc(v->p, new_capacity * v->size);
    if(!new_p) return false;
    v->p = new_p;
    v->capacity = new_capacity;
    return true;
}

size_t vector_push(vector *v, const void *val) {
    if(!v) return 0;
    if(!val) return 0;
    if(!vector_grow(v, v->length + 1)) return 0;
    void *p = (uint8_t*)v->p + (v->size * v->length);
    memcpy(p, val, v->size);
    v->length += 1;
    return v->length;
}

size_t vector_push_i(vector *v, const void *val, size_t *i) {
    if(!v) return 0;
    if(!val) return 0;
    if(!vector_grow(v, v->length + 1)) return 0;
    void *p = (uint8_t*)v->p + (v->size * v->length);
    memcpy(p, val, v->size);
    *i = v->length;
    v->length += 1;
    return v->length;
}

size_t vector_push_n(vector *v, const void *val, size_t count) {
    if(!v) return 0;
    if(!val) return 0;
    if(!vector_grow(v, v->length + count)) return 0;
    void *p = (uint8_t*)v->p + (v->size * v->length);
    memcpy(p, val, v->size * count);
    v->length += count;
    return v->length;
}

bool vector_pop(vector *v, void *dst) {
    if(!v) return false;
    if(!v->length) return true;
    if(dst) {
        void *p = (uint8_t*)v->p + (v->size * (v->length - 1));
        memcpy(dst, p, v->size);
    }
    v->length -= 1;
    return true;
}

bool vector_clear(vector *v) {
    if(!v) return false;
    v->length = 0;
    return true;
}

bool vector_contains(vector *v, const void *val) {
    if(!v) return false;
    if(!val) return false;
    for(size_t i = 0; i < v->length; ++i) {
        void *p = (uint8_t*)v->p + (v->size * i);
        if(memcmp(p, val, v->size) == 0) {
            return true;
        }
    }
    return false;
}

bool vector_push_unique(vector *v, const void *val) {
    if(vector_contains(v, val)) return true;
    return vector_push(v, val);
}


bool vector_find(vector *v, const void *val, size_t *dst) {
    if(!v) return false;
    if(!val) return false;
    for(size_t i = 0; i < v->length; ++i) {
        void *p = (uint8_t*)v->p + (v->size * i);
        if(memcmp(p, val, v->size) == 0) {
            *dst = i;
            return true;
        }
    }
    return false;
}

bool vector_push_unique_i(vector *v, const void *val, size_t *i) {
    if(vector_find(v, val, i)) return true;
    return vector_push_i(v, val, i);
}

#undef VECTOR_IMPLEMENTATION
#endif