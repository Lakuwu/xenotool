#ifndef LEX_FILE_H
#define LEX_FILE_H

#include <stdint.h>

typedef struct {
    char magic[4];
    uint8_t unk0[12];
    //0x10
    char name[32];
    //0x30
    char artist[15];
    uint8_t unk1;
    //0x40
    uint32_t nmatrix;
    uint32_t nmesh;
    uint32_t unk3;
    uint32_t unk4;
    //0x50
    uint32_t addr[5];
    uint8_t unk5[12];
    //0x70
    float max[3];
    float unk6;
    //0x80
    float min[3];
    float unk7;
    //0x90
    float orig[3];
    float unk8;
    //0xa0
    uint8_t unk9[16];
} LexHeader;

typedef struct {
    uint8_t unk0[4];
    uint8_t pal2;
    uint8_t pal;
    uint8_t unk1[10];
} PaletteInfo;

typedef struct {
    uint8_t type;
    union{
        struct {
            uint8_t w : 4;
            uint8_t _ff0 : 4;
            
            uint8_t _ff1 : 3;
            uint8_t x1 : 1;
            uint8_t x : 4;
            
            uint8_t _ff2 : 4;
            uint8_t h : 4;
            
            uint8_t _ff3 : 7;
            uint8_t y1 : 1;
            
            uint8_t y : 4;
            uint8_t _ff4 : 4;
            
            uint8_t unk1[10];
        } type_ff;
        struct {
            uint8_t x : 6;
            uint8_t wlen2 : 2;
            
            uint8_t wlen : 4;
            uint8_t w : 4;
            
            uint8_t _0a0 : 4;
            uint8_t _0a1 : 4;
            
            uint8_t y : 2;
            uint8_t hlen2 : 2;
            uint8_t hlen : 4;
            
            uint8_t h : 4;
            uint8_t y2 : 4;
            
            uint8_t unk1[10];
        } type_0a_;
        struct {
            uint8_t x : 6;
            uint8_t x2 : 2;
            
            uint8_t x1;
            
            uint8_t y;
            
            uint8_t y_ : 2;
            uint8_t y2 : 6;
            
            uint8_t y1;
            
            uint8_t unk1[10];
        } type_0a;
    };
    
} UVInfo;

typedef struct {
    float color0[4];
    float color1[4];
    float color2[4];
} MaterialColor;

typedef struct {
    char bone_name[32];
    //0x20
    uint32_t weight_format;
    uint32_t data_offset;
    uint32_t data_len;
    uint32_t bone_idx;
    //0x30
    uint16_t unk2[32];
    //0x70
    float max[3];
    float unk3;
    //0x80
    float min[3];
    float unk4;
    //0x90
    float orig[3];
    float unk5;
    //0xa0
    uint32_t unk6[8];
    uint8_t vertex_format;
    uint8_t unk6_[15];
    //0xd0
    MaterialColor col;
    float unk7[8];
    PaletteInfo pal0;
    UVInfo uvinfo;
    uint8_t unk8[16];
    //0x150
    char group_name[32];
    //0x170
    char material_name[32];
} MeshHeader;

typedef struct {
    uint32_t unk0[4];
    UVInfo uvinfo;
    PaletteInfo pal0;
    PaletteInfo pal1;
    MaterialColor col;
    uint32_t unk1[4];
} MaterialBlock;

typedef struct {
    uint32_t unk0[4];
    UVInfo uvinfo;
    PaletteInfo pal0;
    PaletteInfo pal1;
} MaterialBlockSmall;

typedef struct {
    union {
        uint16_t imm;
        struct {
            uint8_t imm0;
            uint8_t imm1;
        };
        struct {
            uint16_t addr : 10;
            uint16_t pad0 : 4;
            uint16_t zero_ext : 1;
            uint16_t tops_add : 1;
        };
    };
    uint8_t num;
    union {
        uint8_t cmd;
        struct {
            uint8_t unpack_type : 4;
            uint8_t write_masking : 1;
            uint8_t pad1 : 3;
        };
    };
} VIFCommand;

typedef struct {
    uint8_t type;
    uint8_t format;
    uint8_t length;
    uint8_t unk;
} BlockHeader;


typedef struct {
    uint8_t count;
    uint8_t unk0[3];
    uint8_t format;
    uint8_t unk1[11];
} MeshBlockHeader;

typedef struct {
    MeshHeader header;
} MeshObj;

typedef struct {
    LexHeader header;
    float (*matrix)[16];
    uint32_t *mesh_addr;
    MeshObj *mesh;
} LexFile;

#endif