#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "xenodebug.h"

bool dbgflags[256];

bool dbg(uint8_t flag) {
    return dbgflags[flag];
}

void print_chars(void *src, uint8_t n) {
    char *c = src;
    for(uint8_t i = 0; i < n; ++i) {
        printf("%c", c[i]);
    }
}

void print_bytes(void *src, int n) {
    uint8_t *b = src;
    for(int i = 0; i < n; ++i) {
        printf("%02x ", b[i]);
        if((i+1)%16==0) {
            printf("\n");
        }
    }
}

void print_floats(void *src, int n, int dim) {
    float *f = src;
    int count = 0;
    for(int i = 0; i < n; ++i) {
        printf("% 11.5g ", f[i]);
        ++count;
        if(count == dim) {
            count = 0;
            printf("\n");
        }
    }
}

void print_void(void *src, int n, int dim, size_t size, const char *fmt) {
    int count = 0;
    uint8_t *u8 = src;
    uint16_t *u16 = src;
    uint32_t *u32 = src;
    uint64_t *u64 = src;
    for(int i = 0; i < n; ++i) {
        switch(size) {
            case 1: {printf(fmt, u8[i]); break;}
            case 2: {printf(fmt, u16[i]); break;}
            case 4: {printf(fmt, u32[i]); break;}
            case 8: {printf(fmt, u64[i]); break;}
            default: printf("illegal size ");
        }
        ++count;
        if(count == dim) {
            count = 0;
            printf("\n");
        }
    }
}

void print_bytes_dim(void *src, int n, int dim) {
    uint8_t *b = src;
    int count = 0;
    bool nl = false;
    for(int i = 0; i < n; ++i) {
        nl = true;
        printf("%02x ", b[i]);
        ++count;
        if(count == dim) {
            count = 0;
            printf("\n");
            nl = false;
        }
    }
    if(nl) printf("\n");
}

void print_vertex(Vertex v) {
    printf("p: % 3.3f % 3.3f % 3.3f n: % 3.3f % 3.3f % 3.3f uv: % 1.4f % 1.4f col: %1.2f %1.2f %1.2f %1.2f\n",
    v.x, v.y, v.z, v.nx, v.ny, v.nz, v.u, v.v, v.r, v.g, v.b, v.a);
}

void print_jntheader(JNTHeader h) {
    printf("%s u0: %x block_count: %x pre_blocks: %x u3: %x name_blocks: %x post_blocks: %x\n", h.magic, h.unk0, h.block_count, h.pre_blocks, h.unk3, h.name_blocks, h.post_blocks);
    printf("offset: %x u6: %x u7: %x name: ", h.offset, h.unk6, h.unk7);
    print_chars(h.name, 8);
    printf("\n");
}

void print_jntblock(JNTBlock j) {
    printf("%x %x %x %x %x %x %x %x\n", j.header.type, j.header.unk1, j.header.unk2, j.header.unk3, j.header.unk4, j.header.unk5, j.header.unk6, j.header.unk7);
    switch(j.header.type) {
        case 0x05:
        case 0x06: {
            if(dbg('J')) print_floats(j.f, 8, 4);
            if(dbg('s')) print_chars(j.c + 0x20, 8);
            if(dbg('s')) printf(" ");
            if(dbg('s')) print_chars(j.c + 0x28, 8);
            if(dbg('s') || dbg('J'))printf("\n");
            // print_bytes(j.c, 0x30);
            // print_void(j.c, 0x30, 16, 1, "%02.0x ");
            break;
        }
        
        default: {
            if(dbg('J'))print_floats(j.f, 12, 4);
            // print_void(j.c, 0x30, 16, 1, "%02.0x ");
            // print_bytes(j.c, 0x30);
            break;
        }
    }
    printf("\n");
}

void print_xtxheader(XTXHeader h) {
    printf("%s %d bytes, %d images, start addr: %08x\n", h.magic, h.size, h.count, h.img_header_addr);
}

void print_xtximgheader(XTXImgHeader h) {
    printf("w: %d, h: %d, TBW: %d, img addr: %08x\nunk0: %02x %02x , img_size: %08x, offset: %08x\n",
           h.width, h.height, h.buffer_width, h.img_addr, h.unk0[0], h.unk0[1],
           h.img_size, h.offset);
}

void print_material(Material mat) {
    printf("U (%d, %d) (%f, %f)\n", mat.umin, mat.umax, mat.uminf, mat.umaxf);
    printf("V (%d, %d) (%f, %f)\n", mat.vmin, mat.vmax, mat.vminf, mat.vmaxf);
    if(mat.pal == 0xff) {
        printf("RGBA\n");
    } else {
        printf("Palette %02x (%d, %d)\n", mat.pal, mat.palx, mat.paly);
    }
    print_materialcolor(mat.col);
}

void print_materialraw(MaterialRaw mr) {
    print_uvinfo(mr.uvinfo);
    print_paletteinfo(mr.pal0);
}

void print_lexheader(LexHeader h) {
    printf("%s by %s, %d matrices and %d meshes\n", h.name, h.artist, h.nmatrix, h.nmesh);
    printf("data addresses: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n", h.addr[0], h.addr[1], h.addr[2], h.addr[3], h.addr[4]);
    printf("max % 4.7f % 4.7f % 4.7f\n", h.max[0], h.max[1], h.max[2]);
    printf("min % 4.7f % 4.7f % 4.7f\n", h.min[0], h.min[1], h.min[2]);
    printf("org % 4.7f % 4.7f % 4.7f\n", h.orig[0], h.orig[1], h.orig[2]);
    printf("unk0: ");
    print_bytes(h.unk0, 12);
    printf("\nunk1: %02x unk3: %08x unk4: %08x\n", h.unk1, h.unk3, h.unk4);
    printf("unk5: ");
    print_bytes(h.unk5, 12);
    printf("\nunk6: % 4.7f unk7: % 4.7f unk8: % 4.7f\nunk9: ", h.unk6, h.unk7, h.unk8);
    print_bytes(h.unk9, 16);
    printf("\n\n");
}

void print_meshblockheader(MeshBlockHeader h) {
    printf("\nMeshBlockHeader:\n");
    printf("vertex count: %d, ?: %d\n", h.count, h.format);
    printf("unk0: ");
    print_bytes(h.unk0, 3);
    printf("\nunk1: ");
    print_bytes(h.unk1, 11);
    printf("\n");
}

void print_materialcolor(MaterialColor col) {
    //printf("\nMaterialColor\n");
    print_floats(col.color0, 4, 4);
    print_floats(col.color1, 4, 4);
    print_floats(col.color2, 4, 4);
}

void print_paletteinfo(PaletteInfo pal) {
    //printf("PaletteInfo\n");
    printf("unk0: ");
    print_bytes(pal.unk0, 4);
    printf(" pal2: %02x  pal: %02x  unk1: ", pal.pal2, pal.pal);
    print_bytes(pal.unk1, 10);
    printf("\n");
}

void print_uvinfo(UVInfo uv) {
    //printf("UVInfo\n");
    // printf("unk0: %02x w: %02x x: %02x h: %02x y_offset: %02x y: %02x\nunk1: ",
            // uv.unk0, uv.w, uv.x, uv.h, uv.y_offset, uv.y);
    // print_bytes(uv.unk1, 10);
    print_bytes(&uv, sizeof(uv));
    printf("\n");
}

void print_meshheader(MeshHeader h) {
    printf("bone name: %s\ngroup name: %s\nmaterial name: %s\n", h.bone_name, h.group_name, h.material_name);
    printf("data offset: 0x%08x, data length: 0x%08x\n", h.data_offset, h.data_len);
    printf("max % 4.7f % 4.7f % 4.7f\n", h.max[0], h.max[1], h.max[2]);
    printf("min % 4.7f % 4.7f % 4.7f\n", h.min[0], h.min[1], h.min[2]);
    printf("org % 4.7f % 4.7f % 4.7f\n", h.orig[0], h.orig[1], h.orig[2]);
    printf("weight_format: %08x bone_idx: %08x\n", h.weight_format, h.bone_idx);
    printf("unk2:\n");
    // print_bytes((void*)h.unk2, 16*4);
    print_void(h.unk2, 32, 8, sizeof(uint16_t), "% 4d");
    printf("\n");
    printf("unk3: % 4.7f unk4: % 4.7f unk5: % 4.7f\n", h.unk3, h.unk4, h.unk5);
    printf("unk6:\n");
    print_bytes((void*)h.unk6, 8*4);
    printf("\n");
    printf("vertex_format: %02x\n", h.vertex_format);
    print_bytes(h.unk6_, 15);
    printf("\n");
    print_materialcolor(h.col);
    printf("unk7:\n");
    print_floats(h.unk7, 8, 4);
    print_paletteinfo(h.pal0);
    print_uvinfo(h.uvinfo);
    printf("unk8:\n");
    print_bytes((void*)h.unk8, 16);
    printf("\n\n");
}

void print_materialblock(MaterialBlock mb) {
    printf("MaterialBlock\n");
    print_uvinfo(mb.uvinfo);
    print_paletteinfo(mb.pal0);
    print_paletteinfo(mb.pal1);
    print_materialcolor(mb.col);
    printf("unk0: ");
    print_bytes(mb.unk0, 16);
    printf("\nunk1: ");
    print_bytes(mb.unk1, 16);
    printf("\n");
}

void print_materialblocksmall(MaterialBlockSmall mb) {
    printf("MaterialBlock\n");
    print_uvinfo(mb.uvinfo);
    print_paletteinfo(mb.pal0);
    print_paletteinfo(mb.pal1);
    printf("unk0: ");
    print_bytes(mb.unk0, 16);
    printf("\n");
}

void print_vifcommand(VIFCommand vif, size_t offset) {
    if(vif.cmd >= 0x60) {
        printf("\n[0x%08llx] VIF addr: %03x zext: %d tops-add: %d num: %02x unpack_type: %x write_masking: %d (cmd: %02x)\n",
            offset, vif.addr, vif.zero_ext, vif.tops_add, vif.num, vif.unpack_type,
            vif.write_masking, vif.cmd);
    } else {
        printf("\n[0x%08llx] VIF i0: %02x i1: %02x num: %02x cmd: %02x\n", offset, vif.imm0, vif.imm1, vif.num, vif.cmd);
    }
}