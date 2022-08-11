#ifndef JNT_FILE_H
#define JNT_FILE_H

#include <stdint.h>

typedef struct {
    char magic[4];
    uint16_t unk0;
    uint16_t block_count;
    uint16_t pre_blocks;
    uint16_t unk3;
    uint16_t name_blocks;
    uint16_t post_blocks;
    
    uint32_t offset;
    uint16_t unk6;
    uint16_t unk7;
    char name[8];
} JNTHeader;

typedef struct {
    uint16_t type;
    uint16_t unk1;
    uint16_t unk2;
    uint16_t unk3;
    uint16_t unk4;
    uint16_t unk5;
    uint16_t unk6;
    uint16_t unk7;
} JNTBlockHeader;

typedef struct {
    JNTBlockHeader header;
    union {
        float f[12];
        char c[0x30];
    };
} JNTBlock;
#endif