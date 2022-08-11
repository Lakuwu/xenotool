#ifndef GLB_HEADER
#define GLB_HEADER

#include <stdint.h>

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t length;
} glb_file_header;

#define GLB_MAGIC 0x46546C67
#define GLB_CHUNK_TYPE_JSON 0x4E4F534A
#define GLB_CHUNK_TYPE_BIN 0x004E4942

typedef struct {
    uint32_t chunk_length;
    uint32_t chunk_type;
} glb_chunk_header;

typedef enum {
    GLB_SIGNED_BYTE = 5120,
    GLB_UNSIGNED_BYTE = 5121,
    GLB_SIGNED_SHORT = 5122,
    GLB_UNSIGNED_SHORT = 5123,
    GLB_UNSIGNED_INT = 5125,
    GLB_FLOAT = 5126
} glb_component_type;

typedef enum {
    GLB_ARRAY_BUFFER = 34962,
    GLB_ELEMENT_ARRAY_BUFFER = 34963
} glb_bufferview_target_type;
#endif