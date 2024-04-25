#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef float float32_t;
typedef double float64_t;

typedef enum {
    MsgMove_Type = 1,
} MsgMove_;

struct MsgMove {
    uint16_t x;
    uint16_t y;
    uint16_t theta;
};

static inline size_t parse_MsgMove(MsgMove* __restrict__ out, const char* __restrict__ src, size_t size) {
    if (size < 6) return 0;
    memcpy(&out->x, src, sizeof(out->x));
    src += sizeof(out->x);
    memcpy(&out->y, src, sizeof(out->y));
    src += sizeof(out->y);
    memcpy(&out->theta, src, sizeof(out->theta));
    src += sizeof(out->theta);
    return 6;
}

static inline size_t dump_MsgMove(MsgMove* __restrict__ obj, char* __restrict__ buff, size_t size) {
    if (size < 6) return 0;
    memcpy(buff, &obj->x, sizeof(obj->x));
    buff += sizeof(obj->x);
    memcpy(buff, &obj->y, sizeof(obj->y));
    buff += sizeof(obj->y);
    memcpy(buff, &obj->theta, sizeof(obj->theta));
    buff += sizeof(obj->theta);
    return 6;
}
