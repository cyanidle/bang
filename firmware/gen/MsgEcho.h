#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef float float32_t;
typedef double float64_t;

typedef enum {
    MsgEcho_Type = 9,
} MsgEcho_;

struct MsgEcho {
    uint16_t type;
    uint32_t size;
};

static inline size_t parse_MsgEcho(MsgEcho* __restrict__ out, const char* __restrict__ src, size_t size) {
    if (size < 6) return 0;
    memcpy(&out->type, src, sizeof(out->type));
    src += sizeof(out->type);
    memcpy(&out->size, src, sizeof(out->size));
    src += sizeof(out->size);
    return 6;
}

static inline size_t dump_MsgEcho(const MsgEcho* __restrict__ obj, char* __restrict__ buff, size_t size) {
    if (size < 6) return 0;
    memcpy(buff, &obj->type, sizeof(obj->type));
    buff += sizeof(obj->type);
    memcpy(buff, &obj->size, sizeof(obj->size));
    buff += sizeof(obj->size);
    return 6;
}
