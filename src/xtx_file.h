#ifndef XTX_FILE_H
#define XTX_FILE_H

#include <stdint.h>

typedef struct {
    char magic[4];
    uint32_t size;
    uint32_t count;
    uint32_t img_header_addr;
} XTXHeader;

typedef struct {
    uint8_t unk0[32];
} XTXImgHeader2;

typedef struct {
    uint16_t width;
    uint16_t buffer_width;
    uint16_t height;
    uint8_t unk0[2];
    uint32_t offset;
    uint32_t img_size;
    uint32_t img_addr;
} XTXImgHeader;

typedef struct {
    XTXHeader header;
    XTXImgHeader *img_header;
    XTXImgHeader2 *img_header2;
    uint8_t **img;
} XTXFile;

#endif