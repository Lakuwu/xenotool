#ifndef STR_H
#define STR_H

#include "vector.h"

typedef vector str;

str *str_new();
void str_free(str **s);
size_t str_append_cstr(str *s, const char *c);
const char *str_cstr(str *s);
#endif

#ifdef STR_IMPLEMENTATION

#include <string.h>

str *str_new() {
    str *ret = vector_new(1);
    return ret;
}

void str_free(str **s) {
    vector_free(s);
}

size_t str_append_cstr(str *s, const char *c) {
    size_t len = strlen(c);
    return vector_push_n(s, c, len);
}

const char *str_cstr(str *s) {
    char null = '\0';
    vector_push(s, &null);
    vector_pop(s, NULL);
    return s->p;
}

#undef STR_IMPLEMENTATION
#endif