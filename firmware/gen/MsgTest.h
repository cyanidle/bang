#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef float float32_t;
typedef double float64_t;

typedef enum {
    MsgTest_Type = 7,
} MsgTest_;

struct MsgTest {
    uint8_t led;
};

static inline size_t parse_MsgTest(MsgTest* __restrict__ out, const char* __restrict__ src, size_t size) {
    if (size < 1) return 0;
    memcpy(&out->led, src, sizeof(out->led));
    src += sizeof(out->led);
    return 1;
}

static inline size_t dump_MsgTest(MsgTest* __restrict__ obj, char* __restrict__ buff, size_t size) {
    if (size < 1) return 0;
    memcpy(buff, &obj->led, sizeof(obj->led));
    buff += sizeof(obj->led);
    return 1;
}
