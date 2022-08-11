#ifndef XENODEBUG_H
#define XENODEBUG_H
#include <stdint.h>
#include "jnt_file.h"
#include "lex_file.h"
#include "xtx_file.h"
#include "xenotool.h"

bool dbg(uint8_t flag);
void print_chars(void *src, uint8_t n);
void print_bytes(void *src, int n);
void print_floats(void *src, int n, int dim);
void print_void(void *src, int n, int dim, size_t size, const char *fmt);
void print_bytes_dim(void *src, int n, int dim);

void print_vertex(Vertex v);

void print_jntheader(JNTHeader h);
void print_jntblock(JNTBlock j);

void print_xtxheader(XTXHeader h);
void print_xtximgheader(XTXImgHeader h);

void print_material(Material mat);
void print_materialraw(MaterialRaw mr);
void print_lexheader(LexHeader h);
void print_meshblockheader(MeshBlockHeader h);
void print_materialcolor(MaterialColor col);
void print_paletteinfo(PaletteInfo pal);
void print_uvinfo(UVInfo uv);
void print_meshheader(MeshHeader h);
void print_materialblock(MaterialBlock mb);
void print_materialblocksmall(MaterialBlockSmall mb);
void print_vifcommand(VIFCommand vif, size_t offset);

#endif