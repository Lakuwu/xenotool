#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "arx_file.h"

void print_u64b(uint64_t b) {
    for(int i = 0; i < 64; ++i) {
        printf("%llu", (b >> (63 - i)) & 0x1);
        if(i % 4 == 3) printf(" ");
    }
    puts("");
}

void print_u32b(uint32_t b) {
    for(int i = 0; i < 32; ++i) {
        printf("%u", (b >> (31 - i)) & 0x1);
        if(i % 4 == 3) printf(" ");
    }
    puts("");
}

size_t uncompress_arx(char *filename, void **out) {
    FILE *fp = fopen(filename, "rb");
    if(!fp) return 0;
    ARXHeader h;
    fread(&h, sizeof(ARXHeader), 1, fp);
    *out = malloc(h.size_orig);
    memset(*out, 0, h.size_orig);
    uint32_t *o = *out;
    
    uint64_t buf = 0;
    uint8_t buf_len = 0;
    const uint64_t top = 1llu << 63;
    enum state {
        ARX_DATA,
        ARX_MARKER,
        ARX_LUT
    } s = ARX_DATA;
    uint8_t lut_val, lut_idx, lut_len;
    while(true) {
        uint32_t val;
        fread(&val, sizeof(uint32_t), 1, fp);
        if(feof(fp) || ferror(fp)) break;
        buf |= (uint64_t)val << (32 - buf_len);
        buf_len += 32;
        while(buf_len) {
            uint8_t bit = (buf & top) >> 63;
            switch(s) {
                case ARX_DATA: {
                    if(bit) {
                        s = ARX_MARKER;
                    } else {
                        fread(o++, sizeof(uint32_t), 1, fp);
                        buf <<= 1;
                        --buf_len;
                    }
                    break;
                }
                case ARX_MARKER: {
                    lut_idx = 0;
                    lut_len = 0;
                    lut_val = 0;
                    s = ARX_LUT;
                    buf <<= 1;
                    --buf_len;
                    break;
                }
                case ARX_LUT: {
                    lut_val = (lut_val << 1) | bit;
                    if(lut_idx == 0) lut_len = (bit ? 4 : 2);
                    if(lut_idx == 1 && lut_len == 4 && bit) lut_len = 6;
                    if(lut_idx == 2 && lut_len == 6 && bit) lut_len = 8;
                    ++lut_idx;
                    if(lut_idx == lut_len) {
                        s = ARX_DATA;
                        uint8_t idx = 0;
                        switch(lut_len) {
                            case 2: idx = lut_val; break;
                            case 4: idx = 2 + (lut_val & 0x7); break;
                            case 6: idx = 6 + (lut_val & 0xf); break;
                            case 8: idx = 14 + (lut_val & 0x1f); break;
                        }
                        *o++ = h.lut[idx];
                    }
                    buf <<= 1;
                    --buf_len;
                    break;
                }
            }
        }
    }
    fclose(fp);
    return h.size_orig;
}