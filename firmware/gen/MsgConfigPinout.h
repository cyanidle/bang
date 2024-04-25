#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef float float32_t;
typedef double float64_t;

typedef enum {
    MsgConfigPinout_Type = 8,
} MsgConfigPinout_;

struct MsgConfigPinout {
    int8_t num;
    int8_t encoderA;
    int8_t encoderB;
    int8_t enable;
    int8_t fwd;
    int8_t back;
};

static inline size_t parse_MsgConfigPinout(MsgConfigPinout* __restrict__ out, const char* __restrict__ src, size_t size) {
    if (size < 6) return 0;
    memcpy(&out->num, src, sizeof(out->num));
    src += sizeof(out->num);
    memcpy(&out->encoderA, src, sizeof(out->encoderA));
    src += sizeof(out->encoderA);
    memcpy(&out->encoderB, src, sizeof(out->encoderB));
    src += sizeof(out->encoderB);
    memcpy(&out->enable, src, sizeof(out->enable));
    src += sizeof(out->enable);
    memcpy(&out->fwd, src, sizeof(out->fwd));
    src += sizeof(out->fwd);
    memcpy(&out->back, src, sizeof(out->back));
    src += sizeof(out->back);
    return 6;
}

static inline size_t dump_MsgConfigPinout(const MsgConfigPinout* __restrict__ obj, char* __restrict__ buff, size_t size) {
    if (size < 6) return 0;
    memcpy(buff, &obj->num, sizeof(obj->num));
    buff += sizeof(obj->num);
    memcpy(buff, &obj->encoderA, sizeof(obj->encoderA));
    buff += sizeof(obj->encoderA);
    memcpy(buff, &obj->encoderB, sizeof(obj->encoderB));
    buff += sizeof(obj->encoderB);
    memcpy(buff, &obj->enable, sizeof(obj->enable));
    buff += sizeof(obj->enable);
    memcpy(buff, &obj->fwd, sizeof(obj->fwd));
    buff += sizeof(obj->fwd);
    memcpy(buff, &obj->back, sizeof(obj->back));
    buff += sizeof(obj->back);
    return 6;
}
