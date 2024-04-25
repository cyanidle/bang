#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef float float32_t;
typedef double float64_t;

typedef enum {
    MsgConfigServo_Type = 6,
} MsgConfigServo_;

struct MsgConfigServo {
    int16_t num;
    int16_t channel;
    int16_t speed;
    int16_t minVal;
    int16_t maxVal;
    uint8_t startPercents;
};

static inline size_t parse_MsgConfigServo(MsgConfigServo* __restrict__ out, const char* __restrict__ src, size_t size) {
    if (size < 11) return 0;
    memcpy(&out->num, src, sizeof(out->num));
    src += sizeof(out->num);
    memcpy(&out->channel, src, sizeof(out->channel));
    src += sizeof(out->channel);
    memcpy(&out->speed, src, sizeof(out->speed));
    src += sizeof(out->speed);
    memcpy(&out->minVal, src, sizeof(out->minVal));
    src += sizeof(out->minVal);
    memcpy(&out->maxVal, src, sizeof(out->maxVal));
    src += sizeof(out->maxVal);
    memcpy(&out->startPercents, src, sizeof(out->startPercents));
    src += sizeof(out->startPercents);
    return 11;
}

static inline size_t dump_MsgConfigServo(MsgConfigServo* __restrict__ obj, char* __restrict__ buff, size_t size) {
    if (size < 11) return 0;
    memcpy(buff, &obj->num, sizeof(obj->num));
    buff += sizeof(obj->num);
    memcpy(buff, &obj->channel, sizeof(obj->channel));
    buff += sizeof(obj->channel);
    memcpy(buff, &obj->speed, sizeof(obj->speed));
    buff += sizeof(obj->speed);
    memcpy(buff, &obj->minVal, sizeof(obj->minVal));
    buff += sizeof(obj->minVal);
    memcpy(buff, &obj->maxVal, sizeof(obj->maxVal));
    buff += sizeof(obj->maxVal);
    memcpy(buff, &obj->startPercents, sizeof(obj->startPercents));
    buff += sizeof(obj->startPercents);
    return 11;
}
