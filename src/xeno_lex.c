#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "xeno_lex.h"
#include "xenotool.h"
#include "xenodebug.h"
#include "lex_file.h"
#include "vector.h"
#include "macro.h"

#define MAX_J 8
#define MAX_V 256

float fmod_range(float val, float min, float max) {
    val = fmodf(val - min, max - min) + min;
    if(val < min) val += max-min;
    
    return val;
}

void fmod_range_vertex(Vertex *v0, Vertex *v1, Vertex *v2, float umin, float umax, float vmin, float vmax) {
    float udelta = umax - umin;
    float vdelta = vmax - vmin;
    while(v0->u >= umax && v1->u >= umax && v2->u >= umax && umin < umax) {
        v0->u -= udelta;
        v1->u -= udelta;
        v2->u -= udelta;
    }
    while(v0->u < umin && v1->u < umin && v2->u < umin && umin < umax) {
        v0->u += udelta;
        v1->u += udelta;
        v2->u += udelta;
    }
    while(v0->v >= vmax && v1->v >= vmax && v2->v >= vmax && vmin < vmax) {
        v0->v -= vdelta;
        v1->v -= vdelta;
        v2->v -= vdelta;
    }
    while(v0->v < vmin && v1->v < vmin && v2->v < vmin && vmin < vmax) {
        v0->v += vdelta;
        v1->v += vdelta;
        v2->v += vdelta;
    }
}

int64_t parse_lex(char *filename, Model *model, Texture *tex) {
    FILE *fp = fopen(filename, "rb");
    LexFile lex;
    fread(&lex.header, sizeof(LexHeader), 1, fp);
    if(dbg('h')) print_lexheader(lex.header);
    if(!model->name[0]) {
        memcpy(model->name, lex.header.name + 1, 32);
        model->name[32] = 0;
    }
    
    lex.mesh_addr = malloc(lex.header.nmesh * sizeof(uint32_t));
    lex.mesh = malloc(lex.header.nmesh * sizeof(MeshObj));
    
    fread(lex.mesh_addr, sizeof(uint32_t), lex.header.nmesh, fp);
    lex.matrix = malloc(lex.header.nmatrix * 2 * sizeof(float[16]));
    
    fseek(fp, lex.header.addr[0], SEEK_SET);
    for(uint32_t i = 0; i < lex.header.nmatrix * 2; ++i) {
        fread(lex.matrix[i], sizeof(float[16]), 1, fp);
        if(dbg('h')) printf("\n%x:\n", i);
        if(dbg('h')) print_floats(lex.matrix[i], 16, 4);
    }
    uint8_t *mem = malloc(4096);
    memset(mem, 0, 4096);
    size_t data_len = 0;
    size_t j = 0;
    uint8_t *p[MAX_J];
    size_t num[MAX_J];
    size_t components[MAX_J];
    size_t component_bytes[MAX_J];
    int64_t tricount = 0;
    vector *boneiv = vector_new(sizeof(uint32_t));
    Vertex *vv = malloc(sizeof(Vertex) * MAX_V);
    size_t *vi = malloc(sizeof(size_t) * MAX_V);
    size_t material_index = 0;
    for(uint32_t i = 0; i < lex.header.nmesh; ++i) {
        if(dbg('H')) printf("object %d\n", i);
        fseek(fp, lex.mesh_addr[i], SEEK_SET);
        fread(&lex.mesh[i].header, sizeof(MeshHeader), 1, fp);
        vector_push_unique(boneiv, &(lex.mesh[i].header.bone_idx));
        if(dbg('H')) print_meshheader(lex.mesh[i].header);
        MaterialRaw mr = (MaterialRaw){.uvinfo = lex.mesh[i].header.uvinfo, .pal0 = lex.mesh[i].header.pal0};
        if(dbg('m')) print_materialraw(mr);
        MaterialColor col = lex.mesh[i].header.col;
        Material mat = parse_materialraw(mr, tex);
        mat.col = col;
        vector_push_unique_i(model->material, &mat, &material_index);
        if(dbg('m')) printf("^ material idx %llu\n", material_index);
        Mesh mesh;
        mesh.tri = vector_new(sizeof(Triangle));
        mesh.name[0] = '\0';
        mesh.weight_format = lex.mesh[i].header.weight_format;
        snprintf(mesh.name, 64, "%02d/%s/%s", i, lex.mesh[i].header.group_name, lex.mesh[i].header.bone_name);
        uint32_t next_addr = lex.mesh_addr[i] + lex.mesh[i].header.data_offset + lex.mesh[i].header.data_len;
        fseek(fp, lex.mesh_addr[i] + lex.mesh[i].header.data_offset, SEEK_SET);
        uint32_t write_mask = 0;
        MeshBlockHeader mbh;
        int vals_per_vert;
        uint8_t cl = 0;
        uint8_t wl = 0;
        size_t vertex_count = 0;
        while(true) {
            VIFCommand vif;
            fread(&vif, sizeof(VIFCommand), 1 , fp);
            if(feof(fp) || ftell(fp)-4 == next_addr) {
                break;
            }
            if(dbg('c')) print_vifcommand(vif, ftell(fp));
            bool do_process = false;
            bool do_continue = false;
            switch(vif.cmd) {
                case 0x00: {
                    do_continue = true;
                    break;
                }
                case 0x01: {
                    cl = vif.imm0;
                    wl = vif.imm1;
                    if(dbg('c')) printf("CL: %d WL: %d\n", cl, wl);
                    do_continue = true;
                    break;
                }
                case 0x17: {
                    do_process = true;
                    break;
                }
                case 0x20: {
                    fread(&write_mask, 4, 1, fp);
                    if(dbg('c')) printf("write_mask: %08x\n", write_mask);
                    do_continue = true;
                    break;
                }
                default: {
                    if(vif.cmd >= 0x60 && vif.cmd < 0x80) {
                        if(dbg('c')) printf("VIF unpack\n");
                    } else {
                        printf("\n[0x%08lx] unknown VIF command %02x\n", ftell(fp), vif.cmd);
                        return -1;
                    }
                    break;
                }
            }
            if(do_continue) continue;
            if(do_process) {
                if(dbg('D')) printf("\nthis is after a meshblock\n");
                for(size_t k = 0; k < j; ++k) {
                    if(dbg('D')) printf("k: %llu\n", k);
                    if(component_bytes[k] == 4) {
                        if(dbg('D')) print_floats(p[k], num[k] * components[k], components[k]);
                        if(dbg('D')) print_bytes_dim(p[k], num[k] * components[k] * 4, components[k] * 4);
                    } else if(component_bytes[k] == 1) {
                        if(dbg('D')) print_bytes_dim(p[k], num[k] * components[k], components[k]);
                    } else {
                        printf("oopsie! cant do that :(\n");
                        return -1;
                    }
                }
                vertex_count = mbh.count;
                uint8_t vertex_format = lex.mesh[i].header.vertex_format & 0xf0;
                switch(j) {
                    case 1: {
                        if(vertex_format == 0x80) {
                            p[0] = mem;
                            p[1] = p[0] + vertex_count * 8 * sizeof(float);
                            p[2] = p[1] + vertex_count * 4 * sizeof(float);
                            break;
                        }
                        printf("idk homie (j: %llu)\n", j);
                        return -1;
                    }
                    case 0: {
                        vertex_count = 0;
                        break;
                    }
                    case 2:
                    case 3:
                    case 4:
                    case 5: {
                        
                        break;
                    }
                    default: {
                        printf("idk homie (j: %llu)\n", j);
                        return -1;
                    }
                }
                if(vertex_count == 0) {
                    memset(mem, 0, data_len);
                    data_len = 0;
                    j = 0;
                    continue;
                }
                Material material = ((Material*)model->material->p)[material_index]; 
                for(size_t v = 0; v < vertex_count; ++v) {
                    if(vertex_format == 0x10) {
                        vv[v].x  = ((float*)p[0])[v * 4];
                        vv[v].y  = ((float*)p[0])[v * 4 + 1];
                        vv[v].z  = ((float*)p[0])[v * 4 + 2];
                        vv[v].nx = vv[v].ny = vv[v].nz = 0;
                        vv[v].u  = ((float*)p[0])[v * 4 + 3];
                        vv[v].v  = ((float*)p[1])[v];
                        size_t j_col = (components[2] == 4 ? 2 : 3);
                        vv[v].r  = ((uint8_t*)p[j_col])[v * 4] / 128.0f;
                        vv[v].g  = ((uint8_t*)p[j_col])[v * 4 + 1] / 128.0f;
                        vv[v].b  = ((uint8_t*)p[j_col])[v * 4 + 2] / 128.0f;
                        vv[v].a  = ((uint8_t*)p[j_col])[v * 4 + 3] / 128.0f;
                    } else if(vertex_format == 0x80) {
                        vv[v].x  = ((float*)p[0])[v * 4];
                        vv[v].y  = ((float*)p[0])[v * 4 + 1];
                        vv[v].z  = ((float*)p[0])[v * 4 + 2];
                        vv[v].nx = ((float*)p[0])[v * 4 + vertex_count * 4];
                        vv[v].ny = ((float*)p[0])[v * 4 + vertex_count * 4 + 1];
                        vv[v].nz = ((float*)p[0])[v * 4 + vertex_count * 4 + 2];
                        vv[v].u  = ((float*)p[0])[v * 4 + 3];
                        vv[v].v  = ((float*)p[0])[v * 4 + vertex_count * 4 + 3];
                        vv[v].r  = ((float*)p[1])[v * 4] / 128.0f;
                        vv[v].g  = ((float*)p[1])[v * 4 + 1] / 128.0f;
                        vv[v].b  = ((float*)p[1])[v * 4 + 2] / 128.0f;
                        vv[v].a  = ((float*)p[1])[v * 4 + 3] / 128.0f;
                    } else {
                        printf("[0x%08lx] Error: unknown vertex format! (%02x)\n", ftell(fp), vertex_format);
                        return -1;
                    }
                    switch(lex.mesh[i].header.weight_format) {
                        case 0: // no weights
                            for(int n = 0; n < 4; ++n) {
                                vv[v].w[n] = 0;
                                vv[v].j[n] = -1;
                            }
                            break;
                        case 1: // 8 values per vert: 4 indices and 4 weights
                            for(int n = 0; n < 4; ++n) {
                                vv[v].w[n] = ((float*)p[2])[v * 4 + vertex_count * 4 + n];
                                uint32_t bone = ((uint32_t*)p[2])[v * 4 + n];
                                if(bone) {
                                    uint32_t bi = (bone / 4) - 184;
                                    size_t bii;
                                    uint32_t bn = lex.mesh[i].header.unk2[bi+1];
                                    vector_push_unique_i(model->bone, &bn, &bii);
                                    vv[v].j[n] = bii;
                                } else {
                                    vv[v].j[n] = 0;
                                }
                                
                            }
                            break;
                        case 3: // 4 values per vert, 1 index and 3 zeroes
                            for(int n = 0; n < 4; ++n) {
                                uint32_t bone = ((uint32_t*)p[2])[v * 4 + n];
                                if(bone) {
                                    vv[v].w[n] = 1;
                                    uint32_t bi = (bone / 4) - 184;
                                    size_t bii;
                                    uint32_t bn = lex.mesh[i].header.unk2[bi+1];
                                    vector_push_unique_i(model->bone, &bn, &bii);
                                    vv[v].j[n] = bii;
                                } else {
                                    vv[v].w[n] = ((float*)p[2])[v * 4 + n];;
                                    vv[v].j[n] = 0;
                                }
                                // vv[v].j[n] = n;
                            }
                            break;
                        case 5: // 4 values per vert, 2 indices and 2 weights
                            for(int n = 0; n < 4; ++n) {
                                if(n < 2) {
                                    vv[v].w[n] = ((float*)p[2])[v * 4 + n + 2];
                                    uint32_t bone = ((uint32_t*)p[2])[v * 4 + n];
                                    if(bone) {
                                        uint32_t bi = (bone / 4) - 184;
                                        size_t bii;
                                        uint32_t bn = lex.mesh[i].header.unk2[bi+1];
                                        vector_push_unique_i(model->bone, &bn, &bii);
                                        vv[v].j[n] = bii;
                                    } else {
                                        vv[v].j[n] = 0;
                                    }
                                } else {
                                    vv[v].w[n] = 0;
                                    vv[v].j[n] = 0;
                                }
                            }
                            break;
                        case 1024: // 3 floats per vert, shape key?
                            for(int n = 0; n < 4; ++n) {
                                vv[v].w[n] = 0;
                                vv[v].j[n] = -1;
                            }
                            break;
                        default: {
                            printf("unknown weight_format %d\n", lex.mesh[i].header.weight_format);
                            return -1;
                            break;
                        }
                    }
                }
                for(size_t v = 0; v < vertex_count - 2; ++v) {
                    Vertex vv0 = vv[v];
                    Vertex vv1 = vv[v + 1];
                    Vertex vv2 = vv[v + 2];
                    vv0.v = 1 - vv0.v;
                    vv1.v = 1 - vv1.v;
                    vv2.v = 1 - vv2.v;
                    if(material.has_texture) {
                        fmod_range_vertex(&vv0, &vv1, &vv2, material.uminf, material.umaxf, material.vminf, material.vmaxf);
                    }
                    vv0.v = 1 - vv0.v;
                    vv1.v = 1 - vv1.v;
                    vv2.v = 1 - vv2.v;
                    if(tex) {
                        vv0.v /= 1024/tex->max_y;
                        vv1.v /= 1024/tex->max_y;
                        vv2.v /= 1024/tex->max_y;
                    }
                    vv0.v = 1 - vv0.v;
                    vv1.v = 1 - vv1.v;
                    vv2.v = 1 - vv2.v;
                    vector_push_unique_i(model->vertex, &vv0, &vi[v]);
                    vector_push_unique_i(model->vertex, &vv1, &vi[v + 1]);
                    vector_push_unique_i(model->vertex, &vv2, &vi[v + 2]);
                    Triangle t;
                    t.mat = material_index;
                    size_t v1, v2;
                    if(v % 2 == 0) {
                        v1 = v + 1;
                        v2 = v + 2;
                    } else {
                        v1 = v + 2;
                        v2 = v + 1;
                    }
                    t.i[0] = vi[v];
                    t.i[1] = vi[v1];
                    t.i[2] = vi[v2];
                    vector_push(mesh.tri, &t);
                }
                memset(mem, 0, data_len);
                data_len = 0;
                j = 0;
                continue;
            }
            num[j] = vif.num;
            size_t sizes[] = {4, 2, 1, 0};
            components[j] = (vif.unpack_type >> 2)+1;
            component_bytes[j] = sizes[vif.unpack_type&0x3];
            if(dbg('c')) printf("components %llu component_bytes %llu\n", components[j], component_bytes[j]);
            if(!components[j] || !component_bytes[j]) {
                printf("uhhhhhhhhhhhhhhhhhh\n");
                return -1;
            }
            if(vif.write_masking) {
                if(dbg('c')) printf("write masking not supported, garbage out\n");
                // return -1;
            }
            if(!vif.tops_add) {
                printf("not adding TOPS?! how dare you!\n");
                return -1;
            }
            
            if(vif.addr == 0) {
                fread(&mbh, sizeof(MeshBlockHeader), 1, fp);
                if(dbg('h')) print_meshblockheader(mbh);
                --num[j];
                vals_per_vert = num[j] / mbh.count;
                if(dbg('h')) printf("vals per vertex: %d\n", vals_per_vert);
                if(mbh.unk1[0] == 0x40) {
                    size_t to_read = num[j] * components[j] * component_bytes[j];
                    if(to_read == sizeof(MaterialBlock)) {
                        if(dbg('m')) printf("\n!!!--------- assign new material ---------!!!\n");
                        MaterialBlock matb;
                        fread(&matb, sizeof(MaterialBlock), 1, fp);
                        mr = (MaterialRaw){.uvinfo = matb.uvinfo, .pal0 = matb.pal0};
                        if(dbg('m')) print_materialraw(mr);
                        Material newmat = parse_materialraw(mr, tex);
                        col = matb.col;
                        newmat.col = col;
                        vector_push_unique_i(model->material, &newmat, &material_index);
                        num[j] = 0;
                        if(dbg('m')) print_materialblock(matb);
                        if(dbg('m')) printf("^ material idx %llu\n", material_index);
                    } else if(to_read == sizeof(MaterialBlockSmall)) {
                        if(dbg('m')) printf("\n!!!--------- assign new material without new colors ---------!!!\n");
                        MaterialBlockSmall matbs;
                        fread(&matbs, sizeof(MaterialBlockSmall), 1, fp);
                        mr = (MaterialRaw){.uvinfo = matbs.uvinfo, .pal0 = matbs.pal0};
                        if(dbg('m')) print_materialraw(mr);
                        Material newmat = parse_materialraw(mr, tex);
                        newmat.col = col;
                        vector_push_unique_i(model->material, &newmat, &material_index);
                        num[j] = 0;
                        if(dbg('m')) print_materialblocksmall(matbs);
                        if(dbg('m')) printf("^ material idx %llu\n", material_index);
                    } else {
                        printf("[0x%08lx] how curious... something new!\n", ftell(fp));
                        uint8_t* guff = malloc (num[j] * components[j] * component_bytes[j]);
                        fread(guff, num[j] * components[j] * component_bytes[j], 1, fp);
                        print_bytes_dim(guff, num[j] * components[j] * component_bytes[j], 16);
                        free(guff);
                        return -1;
                    }
                    continue;
                } else if(mbh.unk1[0] != 0x00) {
                    printf("\n[0x%08lx] unknown mbh.unk1[0] %02x\n", ftell(fp), mbh.unk1[0]);
                    return -1;
                }
            }
            size_t to_read = num[j] * components[j] * component_bytes[j];
            //align to 4 byte boundary
            to_read = (to_read + 3) & ~0x3;
            if(dbg('v')) printf("j: %llu addr: %03x, to_read: %03llx, data_len: %03llx\n", j, vif.addr*16, to_read, data_len);
            p[j] = mem + data_len;
            fread(p[j], to_read, 1, fp);
            data_len += to_read;
            ++j;
            if(j > MAX_J) {
                printf("MAX_J exceeded: %llu > %u\n", j, MAX_J);
                return -1;
            }
        }
        tricount += mesh.tri->length;
        if(dbg('t')) printf("%llu triangles\n", mesh.tri->length);
        if(!vector_push(model->mesh, &mesh)) {
            printf("Error: could not push value to mesh vector\n");
            return -1;
        }
    }
    // for(size_t i = 0; i < boneiv->length; ++i) {
        // uint32_t bonei = ((uint32_t*)boneiv->p)[i];
        // printf("[%d]\n", bonei);
    // }
    // for(size_t i = 0; i < model->bone->length; ++i) {
        // uint32_t bone = ((uint32_t*)model->bone->p)[i];
        // printf("%08x %u %d %d %d\n", bone, bone, bone % 4, bone / 4, (bone/4)-184);
        // printf("% 4d  %02x %c \n", bone, bone, bone);
    // }
    if(model->bone->length) printf("%llu weight groups\n", model->bone->length);
    model->bone_count = MAX(model->bone->length, model->bone_count);
    vector_free(&boneiv);
    free(vv);
    free(vi);
    free(mem);
    free(lex.matrix);
    free(lex.mesh);
    free(lex.mesh_addr);
    printf("\n[0x%08lx] Parsing LEX finished!\n", ftell(fp));
    fclose(fp);
    return tricount;
}

Material parse_materialraw_ff(MaterialRaw mr) {
    Material ret = {0};
    uint8_t uvx = mr.uvinfo.type_ff.x;
    uint8_t uvy = mr.uvinfo.type_ff.y;
    uint8_t uvw = mr.uvinfo.type_ff.w;
    uint8_t uvh = mr.uvinfo.type_ff.h;
    bool uvx1 = mr.uvinfo.type_ff.x1;
    bool uvy1 = mr.uvinfo.type_ff.y1;
    ret.umin = uvx * 64 + uvx1 * 32;
    ret.vmin = uvy * 64 + uvy1 * 32;
    ret.umax = ret.umin + (uvw+1) * 16;
    ret.vmax = ret.vmin + (uvh+1) * 16;
    
    return ret;
}

Material parse_materialraw_0aa(MaterialRaw mr) {
    Material ret = {0};
    uint32_t uvx = mr.uvinfo.type_0a_.x;
    uint32_t uvy = mr.uvinfo.type_0a_.y << 4 | mr.uvinfo.type_0a_.y2;
    uint32_t uvw = mr.uvinfo.type_0a_.w;
    uint32_t uvh = mr.uvinfo.type_0a_.h;
    uint32_t wlen = (mr.uvinfo.type_0a_.wlen << 0| mr.uvinfo.type_0a_.wlen2 << 4) + 1;
    uint32_t hlen = (mr.uvinfo.type_0a_.hlen << 0| mr.uvinfo.type_0a_.hlen2 << 4) + 1;
    ret.umin = uvx * 16;
    ret.vmin = uvy * 16;
    ret.umax = ret.umin + (uvw + 1) * (wlen);
    ret.vmax = ret.vmin + (uvh + 1) * (hlen);

    return ret;
}

Material parse_materialraw_0a(MaterialRaw mr) {
    Material ret = {0};
    uint32_t uvx = mr.uvinfo.type_0a.x << 4;
    uint32_t uvy = mr.uvinfo.type_0a.y;
    uint32_t uvx1 = mr.uvinfo.type_0a.x1 << 2 | mr.uvinfo.type_0a.x2;
    uint32_t uvy1 =  mr.uvinfo.type_0a.y1 << 6 | mr.uvinfo.type_0a.y2;
    ret.umin = uvx;
    ret.vmin = uvy;
    ret.umax = uvx1 + 1;
    ret.vmax = uvy1 + 1;

    return ret;
}

Material parse_materialraw(MaterialRaw mr, Texture *tex) {
    Material ret = {0};
    if(dbg('U')) print_uvinfo(mr.uvinfo);
    switch(mr.uvinfo.type) {
        case 0x00: {
            ret.umin = ret.umax = ret.vmin = ret.vmax = 0;
            ret.has_texture = false;
            break;
        }
        // case 0x8a:
        case 0x0a: {
            ret = parse_materialraw_0a(mr);
            ret.has_texture = true;
            break;
        }
        case 0xff: {
            ret = parse_materialraw_ff(mr);
            ret.has_texture = true;
            break;
        }
        default: {
            if(dbg('U')) printf("unknown uv type: ");
            if(dbg('U')) print_uvinfo(mr.uvinfo);
            ret = parse_materialraw_0a(mr);
            ret.has_texture = true;
            if(dbg('!')) ret.umin = ret.umax = ret.vmin = ret.vmax = 0;
            if(dbg('!')) ret.has_texture = false;
            break;
        }
    }
    float width = 1024;
    float height = 256;
    if(tex) {
        width = tex->width;
        height = tex->max_y;
    }
    float pal_mul = mr.pal0.pal == 0xff ? 2 : 1;
    ret.uminf = (float)(ret.umin * pal_mul)/width;
    ret.umaxf = (float)(ret.umax * pal_mul)/width;
    ret.vminf = 1 - ((float)(ret.vmax * pal_mul)/height);
    ret.vmaxf = 1 - ((float)(ret.vmin * pal_mul)/height);
    uint8_t pal_hi = mr.pal0.pal >> 4;
    uint8_t pal_lo = mr.pal0.pal & 0xf;
    ret.palx = (pal_hi%2)*256 + (pal_lo/2)*32 + (mr.pal0.pal2>>7)*16;
    ret.paly = (pal_hi/2)*32  + (pal_lo%2)*16;
    ret.pal = mr.pal0.pal;
    if(dbg('U')) print_material(ret);
    return ret;
}