#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef float float32_t;
typedef double float64_t;

typedef enum {
    MsgConfigMotor_Type = 5,
} MsgConfigMotor_;

struct MsgConfigMotor {
    uint8_t num;
    float32_t radius;
    int32_t angleDegrees;
    float32_t interCoeff;
    float32_t propCoeff;
    float32_t diffCoeff;
    float32_t coeff;
    float32_t turnMaxSpeed;
    float32_t maxSpeed;
    int32_t ticksPerRotation;
    int8_t encoderA;
    int8_t encoderB;
    int8_t enable;
    int8_t fwd;
    int8_t back;
};

static inline size_t parse_MsgConfigMotor(MsgConfigMotor* __restrict__ out, const char* __restrict__ src, size_t size) {
    if (size < 42) return 0;
    memcpy(&out->num, src, sizeof(out->num));
    src += sizeof(out->num);
    memcpy(&out->radius, src, sizeof(out->radius));
    src += sizeof(out->radius);
    memcpy(&out->angleDegrees, src, sizeof(out->angleDegrees));
    src += sizeof(out->angleDegrees);
    memcpy(&out->interCoeff, src, sizeof(out->interCoeff));
    src += sizeof(out->interCoeff);
    memcpy(&out->propCoeff, src, sizeof(out->propCoeff));
    src += sizeof(out->propCoeff);
    memcpy(&out->diffCoeff, src, sizeof(out->diffCoeff));
    src += sizeof(out->diffCoeff);
    memcpy(&out->coeff, src, sizeof(out->coeff));
    src += sizeof(out->coeff);
    memcpy(&out->turnMaxSpeed, src, sizeof(out->turnMaxSpeed));
    src += sizeof(out->turnMaxSpeed);
    memcpy(&out->maxSpeed, src, sizeof(out->maxSpeed));
    src += sizeof(out->maxSpeed);
    memcpy(&out->ticksPerRotation, src, sizeof(out->ticksPerRotation));
    src += sizeof(out->ticksPerRotation);
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
    return 42;
}

static inline size_t dump_MsgConfigMotor(MsgConfigMotor* __restrict__ obj, char* __restrict__ buff, size_t size) {
    if (size < 42) return 0;
    memcpy(buff, &obj->num, sizeof(obj->num));
    buff += sizeof(obj->num);
    memcpy(buff, &obj->radius, sizeof(obj->radius));
    buff += sizeof(obj->radius);
    memcpy(buff, &obj->angleDegrees, sizeof(obj->angleDegrees));
    buff += sizeof(obj->angleDegrees);
    memcpy(buff, &obj->interCoeff, sizeof(obj->interCoeff));
    buff += sizeof(obj->interCoeff);
    memcpy(buff, &obj->propCoeff, sizeof(obj->propCoeff));
    buff += sizeof(obj->propCoeff);
    memcpy(buff, &obj->diffCoeff, sizeof(obj->diffCoeff));
    buff += sizeof(obj->diffCoeff);
    memcpy(buff, &obj->coeff, sizeof(obj->coeff));
    buff += sizeof(obj->coeff);
    memcpy(buff, &obj->turnMaxSpeed, sizeof(obj->turnMaxSpeed));
    buff += sizeof(obj->turnMaxSpeed);
    memcpy(buff, &obj->maxSpeed, sizeof(obj->maxSpeed));
    buff += sizeof(obj->maxSpeed);
    memcpy(buff, &obj->ticksPerRotation, sizeof(obj->ticksPerRotation));
    buff += sizeof(obj->ticksPerRotation);
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
    return 42;
}
