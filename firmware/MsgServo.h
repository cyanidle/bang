#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef float float32_t;
typedef double float64_t;

typedef enum {
    Type = 4,
} MsgServo_;

struct MsgServo {
    int16_t servo;
    int16_t pos;
};

static inline size_t parse_MsgServo(MsgServo* out, const char* __restrict__ src, size_t size) {
    if (size < 4) return 0;
    memcpy(&out->servo, src, sizeof(out->servo));
    src += sizeof(out->servo);
    memcpy(&out->pos, src, sizeof(out->pos));
    src += sizeof(out->pos);
    return 4;
}

static inline size_t dump_MsgServo(MsgServo* obj, char* __restrict__ buff, size_t size) {
    if (size < 4) return 0;
    memcpy(buff, &obj->servo, sizeof(obj->servo));
    buff += sizeof(obj->servo);
    memcpy(buff, &obj->pos, sizeof(obj->pos));
    buff += sizeof(obj->pos);
    return 4;
}
