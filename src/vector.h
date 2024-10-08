#ifndef GOOLIB_VECTOR_H
#define GOOLIB_VECTOR_H

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

vector vector_init(size_t size);
void vector_init_ptr(vector *v, size_t size);
void vector_cleanup(vector *v);
bool vector_grow(vector *v, size_t length);
size_t vector_push(vector *v, const void *val);
size_t vector_push_ptrval(vector *v, const void *val);
size_t vector_push_i(vector *v, const void *val, size_t *i);
size_t vector_push_n(vector *v, const void *val, size_t count);
void *vector_next(vector *v);
size_t vector_reserve(vector *v, size_t count);
bool vector_set(vector *v, const void *val, size_t i);
void vector_get(vector *v, size_t i, void *dst);
void *vector_get_ptr(vector *v, size_t i);
void *vector_get_ptrval(vector *v, size_t i);
void *vector_get_ptr_grow(vector *v, size_t i);
bool vector_pop(vector *v, void *dst);
bool vector_clear(vector *v);
bool vector_contains(vector *v, const void *val);
bool vector_push_unique(vector *v, const void *val);
bool vector_push_unique_i(vector *v, const void *val, size_t *i);
bool vector_find(vector *v, const void *val, size_t *dst);
#endif

#ifdef VECTOR_IMPLEMENTATION

#include "macro.h"

vector vector_init(size_t size) {
    return (vector){.p = NULL, .size = size, .length = 0, .capacity = 0};
}

void vector_init_ptr(vector *v, size_t size) {
    if(!v) return;
    *v = vector_init(size);
}

void vector_cleanup(vector *v) {
    if(!v) return;
    free(v->p);
    *v = (vector){0};
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
    if(!v || !val) return 0;
    if(!vector_grow(v, v->length + 1)) return 0;
    void *p = (char*)v->p + (v->size * v->length);
    memcpy(p, val, v->size);
    v->length += 1;
    return v->length;
}

size_t vector_push_ptrval(vector *v, const void *val) {
    if(!v || !val) return 0;
    if(!vector_grow(v, v->length + 1)) return 0;
    void *p = (char*)v->p + (v->size * v->length);
    memcpy(p, &val, v->size);
    v->length += 1;
    return v->length;
}

size_t vector_push_i(vector *v, const void *val, size_t *i) {
    if(!v || !val) return 0;
    if(!vector_grow(v, v->length + 1)) return 0;
    void *p = (char*)v->p + (v->size * v->length);
    memcpy(p, val, v->size);
    *i = v->length;
    v->length += 1;
    return v->length;
}

size_t vector_push_n(vector *v, const void *val, size_t count) {
    if(!v || !val) return 0;
    if(!vector_grow(v, v->length + count)) return 0;
    void *p = (char*)v->p + (v->size * v->length);
    memcpy(p, val, v->size * count);
    v->length += count;
    return v->length;
}

void *vector_next(vector *v) {
    if(!v) return NULL;
    if(!vector_grow(v, v->length + 1)) return NULL;
    void *p = (char*)v->p + (v->size * v->length);
    v->length += 1;
    return p;
}

size_t vector_reserve(vector *v, size_t count) {
    if(!v) return 0;
    if(!vector_grow(v, v->length + count)) return 0;
    v->length += count;
    return v->length;
}

bool vector_set(vector *v, const void *val, size_t i) {
    if(!v || !val) return false;
    if(!vector_grow(v, i + 1)) return false;
    void *p = (char*)v->p + (v->size * i);
    memcpy(p, val, v->size);
    v->length = MAX(v->length, i + 1);
    return true;
}

void vector_get(vector *v, size_t i, void *dst) {
    if(!v || !v->p || !dst) return;
    if(i >= v->length) return;
    void *p = (char*)v->p + (v->size * i);
    memcpy(dst, p, v->size);
}

void *vector_get_ptr(vector *v, size_t i) {
    if(!v || !v->p) return NULL;
    if(i >= v->length) return NULL;
    void *p = (char*)v->p + (v->size * i);
    return p;
}

void *vector_get_ptrval(vector *v, size_t i) {
    if(!v || !v->p) return NULL;
    if(i >= v->length) return NULL;
    void **p = (void **)((char*)v->p + (v->size * i));
    return *p;
}

void *vector_get_ptr_grow(vector *v, size_t i) {
    if(!v) return NULL;
    if(!vector_grow(v, i + 1)) return NULL;
    void *p = (char*)v->p + (v->size * i);
    v->length = MAX(v->length, i + 1);
    return p;
}

bool vector_pop(vector *v, void *dst) {
    if(!v) return false;
    if(!v->length) return true;
    if(dst) {
        void *p = (char*)v->p + (v->size * (v->length - 1));
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

bool vector_push_unique(vector *v, const void *val) {
    if(vector_contains(v, val)) return true;
    return vector_push(v, val);
}

bool vector_find(vector *v, const void *val, size_t *dst) {
    if(!v || !v->p || !val) return false;
    for(size_t i = 0; i < v->length; ++i) {
        void *p = (char*)v->p + (v->size * i);
        if(memcmp(p, val, v->size) == 0) {
            if(dst) *dst = i;
            return true;
        }
    }
    return false;
}

bool vector_contains(vector *v, const void *val) {
    return vector_find(v, val, NULL);
}

bool vector_push_unique_i(vector *v, const void *val, size_t *i) {
    if(vector_find(v, val, i)) return true;
    return vector_push_i(v, val, i);
}

#undef VECTOR_IMPLEMENTATION
#endif
