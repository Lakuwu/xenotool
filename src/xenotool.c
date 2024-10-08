// gcc -std=c2x -fno-omit-frame-pointer -fcf-protection -fno-math-errno -Wall -Wextra -Wpedantic -g -fsanitize=undefined -fsanitize-trap=all -o ../bin/xenotool.exe xenotool.c xeno_lex.c xeno_xtx.c xeno_arx.c xeno_jnt.c xenodebug.c && xenotool

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define VECTOR_IMPLEMENTATION
#include "vector.h"
#define STR_IMPLEMENTATION
#include "str.h"
#include "jnt_file.h"
#include "lex_file.h"
#include "xtx_file.h"
#include "xenotool.h"
#include "xenodebug.h"
#include "xeno_arx.h"
#include "xeno_jnt.h"
#include "xeno_lex.h"
#include "xeno_xtx.h"
#include "glb.h"
#include "macro.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

extern bool dbgflags[256];

void usage() {
    puts("Usage: xenotool [options] file...");
    puts("Options:");
    puts("  -s            Simulate without writing to file(s)");
    return;
}

XenoFileEnum get_filetype(char *filename) {
    FILE *fp = fopen(filename, "rb");
    if(!fp) return FILE_ERROR;
    XenoFileEnum type;
    
    uint32_t val = 0;
    fread(&val, sizeof(uint32_t), 1, fp);
    switch(val) {
        case FILE_LEX: {
            type = FILE_LEX;
            break;
        }
        case FILE_XTX: {
            type = FILE_XTX;
            break;
        }
        case FILE_JNT: {
            type = FILE_JNT;
            break;
        }
        case FILE_ARX: {
            type = FILE_ARX;
            break;
        }
        default: {
            type = FILE_UNK;
        }
    }
    
    fclose(fp);
    return type;
}

void save_image(char *filename, uint16_t width, uint16_t height, void* src, bool rgb) {
    uint8_t* b = src;
    RGBA *img = malloc(width * height * sizeof(RGBA));
    if(rgb) {
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                uint8_t *p = &b[(y * width * 4) + (x * 4)];
                RGBA *pixel = &img[(y*width)+x];
                pixel->r = p[0];
                pixel->g = p[1];
                pixel->b = p[2];
                pixel->a = CLAMP(255.0f*(((float)p[3])/128.0f), 0, 255);
            }
        }
    } else {
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                uint8_t p = b[(y * width) + (x)];
                RGBA *pixel = &img[(y*width)+x];
                pixel->r = p;
                pixel->g = p;
                pixel->b = p;
                pixel->a = 255;
            }
        }
    }
    stbi_write_png(filename, width, height, 4, img, width * 4);
    free(img);
    printf("Wrote \"%s\"\n", filename);
}

#define TEX_RGB_FMT "%s_RGB.png"
#define TEX_PAL_FMT "%s_palette.png"

void save_mtl(char *mtl_filename, char *xtx_filename, Model *m) {
    FILE *fp = fopen(mtl_filename, "w");
    char *tex_filename = malloc(256);
    if(!fp) return;
    Material *mp = m->material.p;
    for(size_t i = 0; i < m->material.length; ++i) {
        fprintf(fp, "newmtl material_%llu\n", i);
        fprintf(fp, "Kd %6.9f %6.9f %6.9f\n", mp[i].col.color0[0], mp[i].col.color0[1], mp[i].col.color0[2]);
        if(xtx_filename && mp[i].has_texture) {
            if(mp[i].pal == 0xff) {
                snprintf(tex_filename, 256, TEX_RGB_FMT, xtx_filename);
            } else {
                snprintf(tex_filename, 256, TEX_PAL_FMT, xtx_filename);
            }
            fprintf(fp, "map_Kd %s\n", tex_filename);
        }
    }
    free(tex_filename);
    fclose(fp);
}

void save_obj(char *obj_filename, char *mtl_filename, Model *m) {
    FILE *fp = fopen(obj_filename, "w");
    if(!fp) return;
    fprintf(fp,"mtllib %s\n", mtl_filename);
    Vertex *vp = m->vertex.p;
    for(size_t i = 0; i < m->vertex.length; ++i) {
        fprintf(fp, "v %6.9f %6.9f %6.9f\n", vp[i].x, vp[i].y, vp[i].z);
        fprintf(fp, "vt %6.9f %6.9f\n", vp[i].u, vp[i].v);
        fprintf(fp, "vn %6.9f %6.9f %6.9f\n", vp[i].nx, vp[i].ny, vp[i].nz);
    }
    size_t material_index = SIZE_MAX;
    Mesh *mp = m->mesh.p;
    for(size_t i = 0; i < m->mesh.length; ++i) {
        // fprintf(fp, "o object_%llu\n", i);
        fprintf(fp, "o %s\n", mp[i].name);
        Triangle *tp = mp[i].tri.p;
        for(size_t j = 0; j < mp[i].tri.length; ++j) {
            if(tp[j].mat != material_index) {
                material_index = tp[j].mat;
                fprintf(fp, "usemtl material_%llu\n", material_index);
            }
            size_t *idx = tp[j].i;
            if(vp[idx[0]].nx + vp[idx[0]].ny + vp[idx[0]].nz == 0) {
                fprintf(fp, "f %llu/%llu/ %llu/%llu/ %llu/%llu/\n",
                        idx[0]+1,idx[0]+1, idx[1]+1,idx[1]+1, idx[2]+1,idx[2]+1);
            } else {
                fprintf(fp, "f %llu/%llu/%llu %llu/%llu/%llu %llu/%llu/%llu\n",
                        idx[0]+1,idx[0]+1,idx[0]+1, idx[1]+1,idx[1]+1,idx[1]+1, idx[2]+1,idx[2]+1,idx[2]+1);
            }
        }
    }
    fclose(fp);
}

void save_glb(char *glb_filename, char *xtx_filename, Model *m) {
    FILE *fp = fopen(glb_filename, "wb");
    if(!fp) {
        printf("Failed to open file for writing: \"%s\"\n", glb_filename);
        return;
    }
    
    char tex_rgb[256];
    char tex_pal[256];
    if(xtx_filename) {
        char *ch;
        char *p = strrchr(xtx_filename, '/');
        if(!p) {
            p = strrchr(xtx_filename, '\\');
            if(!p) {
                ch = xtx_filename;
            } else {
                ch = p+1;
            }
        } else {
            ch = p+1;
        }
        snprintf(tex_rgb, 256, TEX_RGB_FMT, ch);
        snprintf(tex_pal, 256, TEX_PAL_FMT, ch);
        // printf("%s\n%s\n", tex_rgb, tex_pal);
    }
    
    //prepare binary chunk
    vector bin = vector_init(1);
    Vertex *vp = m->vertex.p;
    float min_x, min_y, min_z, max_x, max_y, max_z;
    min_x = max_x = vp[0].x;
    min_y = max_y = vp[0].y;
    min_z = max_z = vp[0].z;
    for(size_t i = 0; i < m->vertex.length; ++i) {
        vector_push_n(&bin, &vp[i].x, sizeof(float));
        vector_push_n(&bin, &vp[i].y, sizeof(float));
        vector_push_n(&bin, &vp[i].z, sizeof(float));
        min_x = MIN(min_x, vp[i].x);
        min_y = MIN(min_y, vp[i].y);
        min_z = MIN(min_z, vp[i].z);
        max_x = MAX(max_x, vp[i].x);
        max_y = MAX(max_y, vp[i].y);
        max_z = MAX(max_z, vp[i].z);
    }
    size_t position_size = bin.length;
    
    size_t normal_offset = bin.length;
    for(size_t i = 0; i < m->vertex.length; ++i) {
        vector_push_n(&bin, &vp[i].nx, sizeof(float));
        vector_push_n(&bin, &vp[i].ny, sizeof(float));
        vector_push_n(&bin, &vp[i].nz, sizeof(float));
    }
    size_t normal_size = bin.length - normal_offset;
    
    size_t uv_offset = bin.length;
    for(size_t i = 0; i < m->vertex.length; ++i) {
        vector_push_n(&bin, &vp[i].u, sizeof(float));
        float v = 1 - vp[i].v;
        vector_push_n(&bin, &v, sizeof(float));
    }
    size_t uv_size = bin.length - uv_offset;
    
    size_t color_offset = bin.length;
    for(size_t i = 0; i < m->vertex.length; ++i) {
        vector_push_n(&bin, &vp[i].r, sizeof(float));
        vector_push_n(&bin, &vp[i].g, sizeof(float));
        vector_push_n(&bin, &vp[i].b, sizeof(float));
        vector_push_n(&bin, &vp[i].a, sizeof(float));
    }
    size_t color_size = bin.length - color_offset;
    
    size_t weight_offset = bin.length;
    for(size_t i = 0; i < m->vertex.length; ++i) {
        float w[4] = {vp[i].w[0], vp[i].w[1], vp[i].w[2], vp[i].w[3]};
        float sum = w[0] + w[1] + w[2] + w[3];
        for(int j = 0; j < 4; ++j) w[j] /= sum;
        vector_push_n(&bin, &w[0], sizeof(float));
        vector_push_n(&bin, &w[1], sizeof(float));
        vector_push_n(&bin, &w[2], sizeof(float));
        vector_push_n(&bin, &w[3], sizeof(float));
    }
    size_t weight_size = bin.length - weight_offset;
    
    size_t joint_offset = bin.length;
    for(size_t i = 0; i < m->vertex.length; ++i) {
        int16_t j[4] = {vp[i].j[0], vp[i].j[1], vp[i].j[2], vp[i].j[3]};
        float w[4] = {vp[i].w[0], vp[i].w[1], vp[i].w[2], vp[i].w[3]};
        for(int k = 0; k < 4; ++k) if(w[k] == 0 && j[k] != 0) j[k] = 0;
        vector_push_n(&bin, &j[0], sizeof(int16_t));
        vector_push_n(&bin, &j[1], sizeof(int16_t));
        vector_push_n(&bin, &j[2], sizeof(int16_t));
        vector_push_n(&bin, &j[3], sizeof(int16_t));
    }
    size_t joint_size = bin.length - joint_offset;
    
    struct material_span{
        size_t mesh;
        size_t count;
        size_t mat;
    };
    vector mspanv = vector_init(sizeof(struct material_span));
    struct material_span mspan = {0};
    
    size_t indices_offset = bin.length;
    bool has_weights = false;
    for(size_t i = 0; i < m->mesh.length; ++i) {
        Mesh *mesh = &((Mesh*)m->mesh.p)[i];
        has_weights = (mesh->weight_format & 0xff ? true : has_weights);
        Triangle *t = mesh->tri.p;
        for(size_t j = 0; j < mesh->tri.length; ++j) {
            if(mspan.mat == t[j].mat && mspan.mesh == i) {
                ++mspan.count;
            } else {
                vector_push(&mspanv, &mspan);
                mspan.mat = t[j].mat;
                mspan.count = 1;
                mspan.mesh = i;
            }
            uint32_t idx = t[j].i[0];
            vector_push_n(&bin, &idx, sizeof(uint32_t));
            idx = t[j].i[1];
            vector_push_n(&bin, &idx, sizeof(uint32_t));
            idx = t[j].i[2];
            vector_push_n(&bin, &idx, sizeof(uint32_t));
        }
    }
    vector_push(&mspanv, &mspan);
    size_t indices_size = bin.length - indices_offset;
    
    //prepare json chunk
    str json = str_init();
    char buf[1024];
    
    str_append_cstr(&json, "{\"asset\":{\"generator\":\"xenotool by Laku, built on "__DATE__"\",\"version\":\"2.0\"}");
    
    //scene
    str_append_cstr(&json, ",\"scene\":0,\"scenes\":[{\"name\":\"Scene\",\"nodes\":[");
    if(has_weights) {
        snprintf(buf, 1024, "%u", m->bone_count);
        str_append_cstr(&json, buf);
        for(size_t i = 0; i < m->mesh.length; ++i) {
            snprintf(buf, 1024, ",%llu", m->bone.length + 1 + i);
            str_append_cstr(&json, buf);
        }
    } else {
        for(size_t i = 0; i < m->mesh.length; ++i) {
            if(i > 0) str_append_cstr(&json, ",");
            snprintf(buf, 1024, "%llu", i);
            str_append_cstr(&json, buf);
        }
    }
    str_append_cstr(&json, "]}]");
    
    //nodes
    str_append_cstr(&json, ",\"nodes\":[");
    if(has_weights) {
        for(uint32_t i = 0; i < m->bone.length; ++i) {
            if(i > 0) str_append_cstr(&json, ",");
            str_append_cstr(&json, "{");
            if(i > 0) {
                snprintf(buf, 1024, "\"children\":[%d],", i-1);
                str_append_cstr(&json, buf);
            }
            int j = m->bone_count - 1 - i;
            uint32_t bone_idx = ((uint32_t*)m->bone.p)[j];
            snprintf(buf, 1024, "\"name\":\"Bone%02d_%02x\"}", j, bone_idx);
            str_append_cstr(&json, buf);
        }
        snprintf(buf, 1024, ",{\"children\":[%d", m->bone_count - 1);
        str_append_cstr(&json, buf);
        snprintf(buf, 1024, "],\"name\":\"%s\"},", m->name);
        str_append_cstr(&json, buf);
    }
    for(size_t i = 0; i < m->mesh.length; ++i) {
        if(i > 0) str_append_cstr(&json, ",");
        Mesh *mesh = &((Mesh*)m->mesh.p)[i];
        if(mesh->weight_format & 0xff) {
            snprintf(buf, 1024, "{\"mesh\":%llu,\"name\":\"%s\",\"skin\":0}", i, mesh->name);
        } else {
            snprintf(buf, 1024, "{\"mesh\":%llu,\"name\":\"%s\"}", i, mesh->name);
        }
        str_append_cstr(&json, buf);
    }
    str_append_cstr(&json, "]");
    
    //skins
    if(has_weights) {
        str_append_cstr(&json, ",\"skins\":[{\"joints\":[");
        for(uint32_t i = 0; i < m->bone_count; ++i) {
            if(i > 0) str_append_cstr(&json, ",");
            snprintf(buf, 1024, "%d", m->bone_count - 1 - i);
            str_append_cstr(&json, buf);
        }
        str_append_cstr(&json, "],\"name\":\"Armature\"}]");
    }
    
    //materials
    str_append_cstr(&json, ",\"materials\":[");
    for(size_t i = 0; i < m->material.length; ++i) {
        if(i > 0) str_append_cstr(&json, ",");
        Material material = ((Material*)m->material.p)[i];
        float *col = material.col.color0;
        if(xtx_filename && material.has_texture) {
            if(material.pal == 0xff) {
                snprintf(buf, 1024, "{\"alphaMode\":\"OPAQUE\",\"doubleSided\":true,\"name\":\"Material_%llu\",\"pbrMetallicRoughness\":{\"baseColorTexture\":{\"index\":0},\"baseColorFactor\":[%6.9f,%6.9f,%6.9f,%6.9f],\"metallicFactor\":0.0,\"roughnessFactor\":0.5}}", i, CLAMP(col[0],0,1), CLAMP(col[1],0,1), CLAMP(col[2],0,1), CLAMP(col[3],0,1));
            } else {
                snprintf(buf, 1024, "{\"alphaMode\":\"OPAQUE\",\"doubleSided\":true,\"name\":\"Material_%llu\",\"pbrMetallicRoughness\":{\"baseColorTexture\":{\"index\":1},\"baseColorFactor\":[%6.9f,%6.9f,%6.9f,%6.9f],\"metallicFactor\":0.0,\"roughnessFactor\":0.5}}", i, CLAMP(col[0],0,1), CLAMP(col[1],0,1), CLAMP(col[2],0,1), CLAMP(col[3],0,1));
            }
        } else {
            snprintf(buf, 1024, "{\"doubleSided\":true,\"name\":\"Material_%llu\",\"pbrMetallicRoughness\":{\"baseColorFactor\":[%6.9f,%6.9f,%6.9f,%6.9f],\"metallicFactor\":0.0,\"roughnessFactor\":0.5}}", i, CLAMP(col[0],0,1), CLAMP(col[1],0,1), CLAMP(col[2],0,1), CLAMP(col[3],0,1));
        }
        str_append_cstr(&json, buf);
    }
    str_append_cstr(&json, "]");
    
    //meshes
    const int attribute_count = 6;
    size_t mspani = 0;
    struct material_span *mspanp = mspanv.p;
    str_append_cstr(&json, ",\"meshes\":[");
    for(size_t i = 0; i < m->mesh.length; ++i) {
        if(i > 0) str_append_cstr(&json, ",");
        Mesh mesh = ((Mesh*)m->mesh.p)[i];
        snprintf(buf, 1024, "{\"name\":\"%s\",\"primitives\":[", mesh.name);
        str_append_cstr(&json, buf);
        int j = 0;
        while(mspanp[mspani].mesh == i) {
            if(j++ > 0) str_append_cstr(&json, ",");
            switch(mesh.weight_format) {
                default:
                case 0:
                case 1024:
                    snprintf(buf, 1024, "{\"attributes\":{\"POSITION\":0,\"NORMAL\":1,\"TEXCOORD_0\":2,\"COLOR_0\":3},\"indices\":%llu,\"material\":%llu}", mspani+attribute_count, mspanp[mspani].mat);
                    break;
                case 1:
                case 3:
                case 5:
                    snprintf(buf, 1024, "{\"attributes\":{\"POSITION\":0,\"NORMAL\":1,\"TEXCOORD_0\":2,\"COLOR_0\":3,\"WEIGHTS_0\":4,\"JOINTS_0\":5},\"indices\":%llu,\"material\":%llu}", mspani+attribute_count, mspanp[mspani].mat);
                    break;
            }
            
            str_append_cstr(&json, buf);
            ++mspani;
            if(mspani >= mspanv.length) break;
        }
        str_append_cstr(&json, "]}");
    }
    str_append_cstr(&json, "]");
    
    //accessors
    str_append_cstr(&json, ",\"accessors\":[");
    snprintf(buf, 1024, "{\"bufferView\":0,\"componentType\":5126,\"count\":%llu,\"max\":[%6.9f,%6.9f,%6.9f],\"min\":[%6.9f,%6.9f,%6.9f],\"type\":\"VEC3\"},", m->vertex.length, max_x, max_y, max_z, min_x, min_y, min_z);
    str_append_cstr(&json, buf);
    snprintf(buf, 1024, "{\"bufferView\":1,\"componentType\":5126,\"count\":%llu,\"type\":\"VEC3\"},", m->vertex.length);
    str_append_cstr(&json, buf);
    snprintf(buf, 1024, "{\"bufferView\":2,\"componentType\":5126,\"count\":%llu,\"type\":\"VEC2\"},", m->vertex.length);
    str_append_cstr(&json, buf);
    snprintf(buf, 1024, "{\"bufferView\":3,\"componentType\":5126,\"count\":%llu,\"type\":\"VEC4\"},", m->vertex.length);
    str_append_cstr(&json, buf);
    snprintf(buf, 1024, "{\"bufferView\":4,\"componentType\":5126,\"count\":%llu,\"type\":\"VEC4\"},", m->vertex.length);
    str_append_cstr(&json, buf);
    snprintf(buf, 1024, "{\"bufferView\":5,\"componentType\":5123,\"count\":%llu,\"type\":\"VEC4\"},", m->vertex.length);
    str_append_cstr(&json, buf);
    size_t accessor_byteoffset = 0;
    for(size_t i = 0; i < mspanv.length; ++i) {
        if(i > 0) str_append_cstr(&json, ",");
        size_t count = mspanp[i].count * 3;
        snprintf(buf, 1024, "{\"bufferView\":6,\"byteOffset\":%llu,\"componentType\":5125,\"count\":%llu,\"type\":\"SCALAR\"}", accessor_byteoffset, count);
        accessor_byteoffset += count * sizeof(uint32_t);
        str_append_cstr(&json, buf);
    }
    str_append_cstr(&json, "]");
    
    if(xtx_filename) {
        //textures
        str_append_cstr(&json, ",\"textures\":[{\"source\":0},{\"source\":1}]");
        
        //images
        snprintf(buf, 1024, ",\"images\":[{\"uri\":\"%s\"},{\"uri\":\"%s\"}]", tex_rgb, tex_pal);
        str_append_cstr(&json, buf);
    }
    
    //bufferViews
    str_append_cstr(&json, ",\"bufferViews\":[");
    snprintf(buf, 1024, "{\"buffer\":0,\"byteLength\":%llu,\"byteOffset\":0,\"target\":34962},", position_size);
    str_append_cstr(&json, buf);
    snprintf(buf, 1024, "{\"buffer\":0,\"byteLength\":%llu,\"byteOffset\":%llu,\"target\":34962},", normal_size, normal_offset);
    str_append_cstr(&json, buf);
    snprintf(buf, 1024, "{\"buffer\":0,\"byteLength\":%llu,\"byteOffset\":%llu,\"target\":34962},", uv_size, uv_offset);
    str_append_cstr(&json, buf);
    snprintf(buf, 1024, "{\"buffer\":0,\"byteLength\":%llu,\"byteOffset\":%llu,\"target\":34962},", color_size, color_offset);
    str_append_cstr(&json, buf);
    snprintf(buf, 1024, "{\"buffer\":0,\"byteLength\":%llu,\"byteOffset\":%llu,\"target\":34962},", weight_size, weight_offset);
    str_append_cstr(&json, buf);
    snprintf(buf, 1024, "{\"buffer\":0,\"byteLength\":%llu,\"byteOffset\":%llu,\"target\":34962},", joint_size, joint_offset);
    str_append_cstr(&json, buf);
    snprintf(buf, 1024, "{\"buffer\":0,\"byteLength\":%llu,\"byteOffset\":%llu,\"target\":34963}", indices_size, indices_offset);
    str_append_cstr(&json, buf);
    str_append_cstr(&json, "]");
    
    //buffers
    str_append_cstr(&json, ",\"buffers\":[{\"byteLength\":");
    sprintf(buf,"%llu", bin.length);
    str_append_cstr(&json, buf);
    str_append_cstr(&json, "}]");
    
    str_append_cstr(&json, "}");
    
    if(dbg('g')) printf("%s\n", str_cstr(&json));
    
    glb_file_header glbh;
    glbh.magic = GLB_MAGIC;
    glbh.version = 2;
    glb_chunk_header jsonh;
    jsonh.chunk_type = GLB_CHUNK_TYPE_JSON;
    size_t json_padding = ((4 - (json.length % 4)) % 4);
    jsonh.chunk_length = json.length + json_padding;
    glb_chunk_header binh;
    binh.chunk_type = GLB_CHUNK_TYPE_BIN;
    size_t bin_padding = ((4 - (bin.length % 4)) % 4);
    binh.chunk_length = bin.length + bin_padding;
    glbh.length = sizeof(glb_file_header) + (2 * sizeof(glb_chunk_header)) + json.length + bin.length + json_padding + bin_padding;
    
    fwrite(&glbh, sizeof(glb_file_header), 1, fp);
    fwrite(&jsonh, sizeof(glb_chunk_header), 1, fp);
    fwrite(json.p, 1, json.length, fp);
    char json_padding_data[4] = "   ";
    fwrite(json_padding_data, 1, json_padding, fp);
    fwrite(&binh, sizeof(glb_chunk_header), 1, fp);
    fwrite(bin.p, 1, bin.length, fp);
    uint8_t bin_padding_data[3] = {0, 0, 0};
    fwrite(bin_padding_data, 1, bin_padding, fp);
    fclose(fp);
    str_cleanup(&json);
    vector_cleanup(&bin);
    vector_cleanup(&mspanv);
    printf("Wrote \"%s\"\n", glb_filename);
}

int main(int argc, char **argv) {
    // setvbuf(stdout, NULL, _IOLBF, 8192);
    if(argc < 2) {
        usage();
        return 0;
    }
    char *lex_file = NULL, *xtx_file = NULL, *jnt_file = NULL, *arx_file = NULL;
    vector lex_files = vector_init(sizeof(char*));
    bool flag_write = true;
    bool gltf_write = true;
    for(int i = 0; i < 256; ++i) dbgflags[i] = false;
    for(int i = 1; i < argc; ++i) {
        if(argv[i][0] == '-') {
            switch(argv[i][1]) {
                case 's': {
                    flag_write = false;
                    break;
                }
                case 'w': {
                    if(argv[i][2] == 'o') {
                        gltf_write = false;
                    }
                    break;
                }
                case 'D': {
                    char *p = &argv[i][2];
                    if(!*p) for(int i = 0; i < 256; ++i) dbgflags[i] = true;
                    while(*p) {
                        dbgflags[*(uint8_t*)p] = true;
                        ++p;
                    }
                    break;
                }
                default: {
                    usage();
                    return -1;
                }
            }
        } else {
            XenoFileEnum type = get_filetype(argv[i]);
            switch(type) {
                case FILE_LEX: {
                    printf("LEX file \"%s\"\n", argv[i]);
                    if(!lex_file) lex_file = argv[i];
                    vector_push(&lex_files, &argv[i]);
                    break;
                }
                case FILE_XTX: {
                    printf("XTX file \"%s\"\n", argv[i]);
                    if(!xtx_file) {
                        xtx_file = argv[i];
                    } else {
                        puts("Multiple XTX files, exiting.");
                        return -1;
                    }
                    break;
                }
                case FILE_JNT: {
                    printf("JNT file \"%s\"\n", argv[i]);
                    if(!jnt_file) {
                        jnt_file = argv[i];
                    } else {
                        puts("Multiple JNT files, exiting.");
                        return -1;
                    }
                    break;
                }
                case FILE_ARX: {
                    printf("ARX file \"%s\"\n", argv[i]);
                    if(!arx_file) {
                        arx_file = argv[i];
                    } else {
                        puts("Multiple ARX files, exiting.");
                        return -1;
                    }
                    break;
                }
                case FILE_ERROR: {
                    printf("Error: Reading file \"%s\" failed, exiting.\n", argv[i]);
                    return -1;
                }
                default: {
                    printf("Warning: Ignoring unknown file \"%s\"", argv[i]);
                }
            }
        }
    };
    int64_t ret = 0;
    Texture *tex = NULL;
    Model *model = NULL;
    void* arx_data = NULL;
    if(xtx_file) {
        tex = malloc(sizeof(Texture));
        memset(tex, 0, sizeof(Texture));
        ret = parse_xtx(xtx_file, tex);
        if(ret) {
            printf("Failed to parse XTX file \"%s\".\n", xtx_file);
            goto END;
        }
    }
    
    if(lex_files.length) {
        model = malloc(sizeof(Model));
        model->mesh = vector_init(sizeof(Mesh));
        model->vertex = vector_init(sizeof(Vertex));
        model->material = vector_init(sizeof(Material));
        model->bone = vector_init(sizeof(uint32_t));
        model->bone_count = 0;
        model->name[0] = 0;
        for(size_t i = 0; i < lex_files.length; ++i) {
            ret = parse_lex(((char**)lex_files.p)[i], model, tex);
            if(ret < 0) {
                printf("Failed to parse LEX file \"%s\".\n", ((char**)lex_files.p)[i]);
                goto END;
            }
            printf("%lld tris\n", ret);
        }
    }
    
    size_t arx_size = 0;
    if(arx_file) {
        arx_size = uncompress_arx(arx_file, &arx_data);
        if(!arx_size) {
            printf("Failed to uncompress ARX file \"%s\".\n", arx_file);
            goto END;
        }
    }
    
    if(arx_file && flag_write) {
        char filename[256];
        snprintf(filename, 256, "%s_uncomp", arx_file);
        FILE *fp = fopen(filename, "wb");
        if(fp) {
            fwrite(arx_data, 1, arx_size, fp);
            fclose(fp);
            printf("Wrote \"%s\"\n", filename);
        }
    }
    
    if(lex_files.length && flag_write && !gltf_write) {
        char *obj_filename = malloc(256);
        char *mtl_filename = malloc(256);
        snprintf(obj_filename, 256, "%s.obj", lex_file);
        snprintf(mtl_filename, 256, "%s.mtl", lex_file);
        save_obj(obj_filename, mtl_filename, model);
        save_mtl(mtl_filename, xtx_file, model);
        free(obj_filename);
        free(mtl_filename);
    }
    
    if(lex_files.length && flag_write && gltf_write) {
        char *glb_filename = malloc(256);
        snprintf(glb_filename, 256, "%s.glb", lex_file);
        save_glb(glb_filename, xtx_file, model);
        free(glb_filename);
    }
    
    if(xtx_file && flag_write) {
        char filename[256];
        snprintf(filename, 256, "%s_RGB.png", xtx_file);
        save_image(filename, tex->width / 2, tex->height / 2, tex->rgb, true);
        
        snprintf(filename, 256, "%s_unswizzled.png", xtx_file);
        save_image(filename, tex->width, tex->height, tex->unswizzled, false);
        
        if(model->material.length) {
            RGBA *rgba = apply_palettes(tex->rgb, tex->unswizzled, tex->width, tex->height, &model->material);
            snprintf(filename, 256, "%s_palette.png", xtx_file);
            save_image(filename, tex->width, tex->height, rgba, true);
            free(rgba);
        }
    }
    
    if(jnt_file) {
        ret = parse_jnt(jnt_file);
        if(ret < 0) {
            printf("Failed to parse JNT file \"%s\".\n", jnt_file);
        }
    }
END:
    if(arx_data) {
        free(arx_data);
    }
    if(tex) {
        free(tex->rgb);
        free(tex->unswizzled);
        free(tex);
    }
    if(model) {
        vector_cleanup(&(model->material));
        vector_cleanup(&(model->mesh));
        vector_cleanup(&(model->vertex));
        free(model);
    }
    return ret;
}