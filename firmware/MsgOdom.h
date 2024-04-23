#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef float float32_t;
typedef double float64_t;

struct Odom {
    int8_t num;
    int8_t aux;
    uint16_t ddist_mm;
};

static inline size_t parse_Odom(Odom* out, const char* __restrict__ src, size_t size) {
    if (size < 4) return 0;
    memcpy(&out->num, src, sizeof(out->num));
    src += sizeof(out->num);
    memcpy(&out->aux, src, sizeof(out->aux));
    src += sizeof(out->aux);
    memcpy(&out->ddist_mm, src, sizeof(out->ddist_mm));
    src += sizeof(out->ddist_mm);
    return 4;
}
static inline size_t dump_Odom(Odom* obj, char* __restrict__ buff, size_t size) {
    if (size < 4) return 0;
    memcpy(buff, &obj->num, sizeof(obj->num));
    buff += sizeof(obj->num);
    memcpy(buff, &obj->aux, sizeof(obj->aux));
    buff += sizeof(obj->aux);
    memcpy(buff, &obj->ddist_mm, sizeof(obj->ddist_mm));
    buff += sizeof(obj->ddist_mm);
    return 4;
}
