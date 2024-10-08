#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "xtx_file.h"
#include "xenotool.h"
#include "xenodebug.h"
#include "macro.h"

void get_palette(uint8_t *img_rgb, uint16_t x0, uint16_t y0, uint16_t w, RGBA *pal) {
    RGBA *rgba = (RGBA*) img_rgb;
    int i = 0;
    for(uint16_t y = 0; y < 16; ++y) {
        for(uint16_t x = 0; x < 16; ++x) {
            pal[i++] = rgba[((y0 + y) * w) + x0 + x];
        }
    }
    for(int i = 0; i < 8; ++i) {
        for(int j = 0; j < 8; ++j) {
            int k = (i*32) + 8 + j;
            int l = (i*32) + 16 + j;
            RGBA c = pal[k];
            pal[k] = pal[l];
            pal[l] = c;
        }
    }
}

RGBA* apply_palettes(uint8_t *img_rgb, uint8_t *img, uint16_t w, uint16_t h, vector *mat) {
    RGBA *ret = malloc(w * h * sizeof(RGBA));
    memset(ret, 0, w * h * sizeof(RGBA));
    int *mat_pal = malloc(w * h * sizeof(int));
    for(int i = 0; i < w * h ; ++i) mat_pal[i] = -1;
    int64_t *mat_idx = malloc(w * h * sizeof(int64_t));
    for(int i = 0; i < w * h ; ++i) mat_idx[i] = -1;
    RGBA pal[256];
    Material *t = mat->p;
    for(size_t i = 0; i < mat->length; ++i) {
        if(t[i].pal == 0xff) continue;
        if(dbg('m')) printf("Material %llu: \n", i);
        if(dbg('m')) print_material(t[i]);
        get_palette(img_rgb, t[i].palx, t[i].paly, w/2, pal);
        for(uint32_t v = t[i].vmin; v < t[i].vmax; ++v) {
            for(uint32_t u = t[i].umin; u < t[i].umax; ++u) {
                if(u >= (w) || v >= (h)) continue;
                size_t offset = (v * w) + u;
                if(dbg('!') && mat_idx[offset] >= 0 && mat_idx[offset] != (int64_t)i) {
                    printf("idx overlap at %d %d idx: new %lld old %lld pal: new %x old %x\n", u, v, i, mat_idx[offset], t[i].pal, mat_pal[offset]);
                    free(mat_idx);
                    free(mat_pal);
                    return ret;
                }
                if(dbg('!') && mat_pal[offset] >= 0 && mat_pal[offset] != t[i].pal) {
                    printf("palette overlap at %d %d idx: new %lld old %lld pal: new %x old %x\n", u, v, i, mat_idx[offset], t[i].pal, mat_pal[offset]);
                    free(mat_pal);
                    free(mat_idx);
                    return ret;
                }
                mat_pal[offset] = t[i].pal;
                mat_idx[offset] = i;
                ret[offset] = pal[img[offset]];
            }
        }
    }
    free(mat_idx);
    free(mat_pal);
    return ret;
}

// I think I found this somewhere on xentax. Whoever it was that wrote it, thank you.
uint8_t* unswizzle8(uint8_t *b, uint16_t width, uint16_t height) {
    uint8_t *ret = malloc(width * height);
    memset(ret, 0, width * height);
    for(int y = 0; y < height; ++y) {
        for(int x = 0; x < width; ++x) {
            int block_location = (y & (~0xf)) * width + (x & (~0xf)) * 2;
            int swap_selector = (((y + 2) >> 2) & 0x1) * 4;
            int posY = (((y & (~3)) >> 1) + (y & 1)) & 0x7;
            int column_location = posY * width * 2 + ((x + swap_selector) & 0x7) * 4;
            int byte_num = ((y >> 1) & 1) + ((x >> 2) & 2);
            ret[(y * width) + x] = b[block_location + column_location + byte_num];
        }
    }
    return ret;
}

int parse_xtx(char *filename, Texture *tex) {
    XTXFile xtx;
    FILE *fp = fopen(filename, "rb");
    fread(&xtx.header, sizeof(XTXHeader), 1, fp);
    if(dbg('x')) print_xtxheader(xtx.header);
    xtx.img = malloc(xtx.header.count * sizeof(uint8_t*));
    xtx.img_header = malloc(xtx.header.count * sizeof(XTXImgHeader));
    xtx.img_header2 = malloc(xtx.header.count * sizeof(XTXImgHeader2));
    fseek(fp, xtx.header.img_header_addr, SEEK_SET);
    fread(xtx.img_header, sizeof(XTXImgHeader), xtx.header.count, fp);
    uint16_t buffer_width =  xtx.img_header[0].buffer_width;
    for(uint32_t i = 0; i < xtx.header.count; ++i) {
        XTXImgHeader h = xtx.img_header[i];
        if(xtx.img_header[i].buffer_width != buffer_width) {
            printf("Error: mismatch in buffer_width\n");
            return -1;
        }
        if(dbg('x')) print_xtximgheader(h);
        size_t size = h.width * h.height * 4;
        xtx.img[i] = malloc(size);
        fseek(fp, h.img_addr, SEEK_SET);
        fread(&(xtx.img_header2[i]), sizeof(XTXImgHeader2), 1, fp);
        if(dbg('x')) print_bytes_dim(xtx.img_header2[i].unk0, 32, 16);
        fread(xtx.img[i], size, 1, fp);
    }
    fclose(fp);
    if(buffer_width == 0) buffer_width = 8;
    uint32_t len = 0;
    switch(buffer_width) {
        case 4: {
            tex->width = 512;
            tex->height = 512;
            len = 256;
            break;
        }
        case 8: {
            tex->width = 1024;
            tex->height = 1024;
            len = 512;
            break;
        }
        default: {
            printf("Error: unknown buffer_width %d\n", buffer_width);
            return -1;
        }
    }
    tex->rgb = malloc(tex->width * tex->height);
    tex->max_x = 0;
    tex->max_y = 0;
    for(uint32_t i = 0; i < xtx.header.count; ++i) {
        XTXImgHeader h = xtx.img_header[i];
        uint32_t o = h.offset;
        uint32_t block = o / 4096;
        o = o % 4096;
        if(dbg('x')) printf("offset: %d block: %d, remainder: %d\n", h.offset, block, o);
        uint32_t x0 = block % (buffer_width / 2);
        x0 *= 64;
        uint32_t y0 = block / (buffer_width / 2);
        y0 *= 32;
        tex->max_x = MAX(tex->max_x, (x0 + h.width) * 2);
        tex->max_y = MAX(tex->max_y, (y0 + h.height) * 2);
        if(dbg('x')) printf("x: %d, y: %d\n", x0, y0);
        for(uint32_t y = 0; y < h.height; ++y) {
            for(uint32_t x = 0; x < h.width; ++x) {
                size_t offset_src = (y * h.width) + x;
                size_t offset_dst = ((y0 + y) * len) + x0 + x;
                ((RGBA*)tex->rgb)[offset_dst] = ((RGBA*)xtx.img[i])[offset_src];
            }
        }
    }
    if(dbg('x')) printf("max x: %d, max y: %d\n", tex->max_x, tex->max_y);
    tex->unswizzled = unswizzle8(tex->rgb, tex->width, tex->height);
    for(uint32_t i = 0; i < xtx.header.count; ++i) free(xtx.img[i]);
    free(xtx.img);
    free(xtx.img_header);
    free(xtx.img_header2);
    return 0;
}