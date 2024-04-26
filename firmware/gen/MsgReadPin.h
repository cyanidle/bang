#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef float float32_t;
typedef double float64_t;

typedef enum {
    MsgReadPin_Type = 12,
} MsgReadPin_;

struct MsgReadPin {
    int8_t pin;
    int8_t value;
    int8_t pullup;
};

static inline size_t parse_MsgReadPin(MsgReadPin* __restrict__ out, const char* __restrict__ src, size_t size) {
    if (size < 3) return 0;
    memcpy(&out->pin, src, sizeof(out->pin));
    src += sizeof(out->pin);
    memcpy(&out->value, src, sizeof(out->value));
    src += sizeof(out->value);
    memcpy(&out->pullup, src, sizeof(out->pullup));
    src += sizeof(out->pullup);
    return 3;
}

static inline size_t dump_MsgReadPin(const MsgReadPin* __restrict__ obj, char* __restrict__ buff, size_t size) {
    if (size < 3) return 0;
    memcpy(buff, &obj->pin, sizeof(obj->pin));
    buff += sizeof(obj->pin);
    memcpy(buff, &obj->value, sizeof(obj->value));
    buff += sizeof(obj->value);
    memcpy(buff, &obj->pullup, sizeof(obj->pullup));
    buff += sizeof(obj->pullup);
    return 3;
}
