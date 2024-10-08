#ifndef XENOTOOL_H
#define XENOTOOL_H

#include <stdint.h>

#include "vector.h"
#include "lex_file.h"

typedef enum {
    FILE_ERROR = 0,
    FILE_UNK = 1,
    FILE_LEX = 0x0078656c,
    FILE_XTX = 0x00585458,
    FILE_JNT = 0x00544e4a,
    FILE_ARX = 0x00585241
} XenoFileEnum;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} RGBA;

typedef struct {
    UVInfo uvinfo;
    PaletteInfo pal0;
} MaterialRaw;

typedef struct {
    uint32_t umin, umax, vmin, vmax;
    float uminf, umaxf, vminf, vmaxf;
    uint32_t palx, paly;
    MaterialColor col;
    bool has_texture;
    uint8_t pal;
} Material;

typedef struct {
    size_t i[3];
    size_t mat;
} Triangle;

typedef struct {
    float x;
    float y;
    float z;
    float nx;
    float ny;
    float nz;
    float u;
    float v;
    float r;
    float g;
    float b;
    float a;
    float w[4]; //weights
    int16_t j[4]; //joint idx
} Vertex;

typedef struct {
    vector tri;
    char name[64];
    uint32_t weight_format;
} Mesh;

typedef struct {
    vector mesh;
    vector material;
    vector vertex;
    vector bone;
    uint32_t bone_count;
    char name[33];
} Model;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t *rgb;
    uint8_t *unswizzled;
    uint32_t max_x;
    uint32_t max_y;
} Texture;

#endif