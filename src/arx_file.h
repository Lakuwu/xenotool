#ifndef ARX_FILE_H
#define ARX_FILE_H

#include <stdint.h>

typedef struct {
    char magic[4];
    uint32_t size_orig;
    uint32_t size_comp;
    uint32_t unk0;
    uint32_t lut[30];
} ARXHeader;

#endif