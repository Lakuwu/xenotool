#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "xeno_jnt.h"
#include "xenotool.h"
#include "xenodebug.h"
#include "jnt_file.h"
#include "vector.h"
#include "macro.h"

void print_treelines(int n, uint64_t lines) {
    for(int i = 0; i < n; ++i) printf("%c  ", (lines >> i) & 1 ? '|' : ' ');
}

int leaves;
JNTBlock *blockp;
typedef struct {
    float x,y,z;
} f3;
f3 *pos;
void treeprint(int *a, int len, int depth, int root, uint64_t lines) {
    int last_child = -1;
    for(int i = 0; i < len; ++i) if(a[i] == root) last_child = i;
    if(last_child == -1) ++leaves;
    for(int i = 0; i < len; ++i) {
        if(i == root) continue;
        if(a[i] == root) {
            if(i == last_child) {
                lines &= ~(1 << depth);
            } else {
                lines |= 1 << depth;
            }
            print_treelines(depth, lines);
            printf("L_%2x: %d %d", i, blockp[i].header.type, blockp[i].header.unk1);
            if(blockp[i].header.type == 3) {
                f3 _pos = pos[root];
                _pos.x += blockp[i].f[0];
                _pos.y += blockp[i].f[1];
                _pos.z += blockp[i].f[2];
                pos[i] = _pos;
                printf(" => %f %f %f", _pos.x, _pos.y, _pos.z);
            } else if(blockp[i].header.type == 4 && 1) {
                f3 _pos = pos[root];
                _pos.x += blockp[i].f[0];
                _pos.y += blockp[i].f[1];
                _pos.z += blockp[i].f[2];
                pos[i] = _pos;
                printf("   => %f %f %f", _pos.x, _pos.y, _pos.z);
            } else {
                pos[i] = pos[root];
                // printf("  %f %f %f", _pos.x, _pos.y, _pos.z);
            }
            puts("");
            treeprint(a, len, depth + 1, i, lines);
        }
    }
}

/*

2 rotates local axes along which we do operations (maybe)
3 adds to position, according to the rotated axes (maybe!)
4 "exports" bone with, get tip of bone by adding 4's value (maybe!!)

*/

int parse_jnt(char *filename) {
    FILE *fp = fopen(filename, "rb");
    JNTHeader jnt_h;
    fread(&jnt_h, sizeof(JNTHeader), 1, fp);
    print_jntheader(jnt_h);
    size_t extra_len = jnt_h.offset - 0x10;
    uint16_t *extra = malloc(extra_len);
    fread(extra, extra_len, 1, fp);
    if(extra_len) {
        if(dbg('j')) print_void(extra, extra_len / sizeof(uint16_t), 8, sizeof(uint16_t), "% 6u ");
        if(dbg('j')) print_bytes(extra, extra_len);
    }
    JNTBlock *block = malloc(sizeof(JNTBlock) * jnt_h.block_count);
    fread(block, sizeof(JNTBlock), jnt_h.block_count, fp);
    blockp = block;
    // uint16_t counts[0xff];
    // memset(counts, 0, sizeof(uint16_t) * 0xff);
    uint16_t cc[8][8];
    memset(cc, 0, sizeof(cc[0][0]) * 8 * 8);
    int *arr = malloc(sizeof(int) * jnt_h.block_count);
    pos = malloc(sizeof(f3) * jnt_h.block_count);
    float f[3] = {0,0,0};
    for(int i = 0; i < jnt_h.block_count; ++i) {
        arr[i] = block[i].header.unk7;
        if(dbg('j')) printf("JNTBlock %02x: ", i);
        if(dbg('j')) print_jntblock(block[i]);
        // uint8_t j = block[i].header.type;
        // ++counts[j];
        if(block[i].header.type == 3 || block[i].header.type == 4) {
            f[0] += block[i].f[0];
            f[1] += block[i].f[1];
            f[2] += block[i].f[2];
            // printf(" => %f %f %f\n", f[0], f[1], f[2]);
        }
        ++cc[block[i].header.type][block[i].header.unk1];
    }
    // for(int i = 0; i < 256; ++i) {
        // if(counts[i]) printf("%x: %x\n", i, counts[i]);
    // }
    if(dbg('s')) {
        for(int i = 0; i < 8; ++i) {
            for(int j = 0; j < 8; ++j) {
                if(cc[i][j]) printf("%d %d: %d\n", i, j, cc[i][j]);
            }
        }
    }
    leaves = 0;
    pos[0] = (f3){0,0,0};
    if(dbg('T')) treeprint(arr,jnt_h.block_count, 0, 0, 0);
    if(dbg('T')) printf("%d leaves\n", leaves);
    free(arr);
    free(pos);
    uint8_t val = 0;
    fread(&val, 1, 1, fp);
    fclose(fp);
    if(val) {
        printf("%d\n", val);
        return -1;
    }
    if(getc(fp) != EOF) return -1;
    return 0;
}