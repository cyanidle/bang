#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef float float32_t;
typedef double float64_t;

typedef enum {
    Type = 3,
} MsgPid_;

struct MsgPid {
    int8_t motor;
    int32_t p;
    int32_t i;
    int32_t d;
};

static inline size_t parse_MsgPid(MsgPid* out, const char* __restrict__ src, size_t size) {
    if (size < 13) return 0;
    memcpy(&out->motor, src, sizeof(out->motor));
    src += sizeof(out->motor);
    memcpy(&out->p, src, sizeof(out->p));
    src += sizeof(out->p);
    memcpy(&out->i, src, sizeof(out->i));
    src += sizeof(out->i);
    memcpy(&out->d, src, sizeof(out->d));
    src += sizeof(out->d);
    return 13;
}

static inline size_t dump_MsgPid(MsgPid* obj, char* __restrict__ buff, size_t size) {
    if (size < 13) return 0;
    memcpy(buff, &obj->motor, sizeof(obj->motor));
    buff += sizeof(obj->motor);
    memcpy(buff, &obj->p, sizeof(obj->p));
    buff += sizeof(obj->p);
    memcpy(buff, &obj->i, sizeof(obj->i));
    buff += sizeof(obj->i);
    memcpy(buff, &obj->d, sizeof(obj->d));
    buff += sizeof(obj->d);
    return 13;
}
