#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <Arduino.h>
#include "utility.hpp"

#define MAX_PWM 255
#define MAX_INTER_TERM 30000
#define MAX_MOTORS 3


struct ShieldPinout
{
    int encoderA;
    int encoderB;
    int enable;
    int fwd;
    int back;
};

struct MotorParams
{
    float radius;
    int angleDegrees;
    float interCoeff;
    float propCoeff;
    float diffCoeff;
    float coeff = 1;
    float turnMaxSpeed = 0.25;
    float maxSpeed = 0.50; 
    int ticksPerRotation = 360;
};

class Motor
{
public:
    using Callback = void(*)();

    Motor(uint8_t num, Callback cb);
    void SetPinout(ShieldPinout const& params);
    void SetParams(const MotorParams &initStruct);
    float Update();
    void SpeedCallback(float x, float y, float turn) noexcept;

    // TODO: Sepate fields into anon structs
    volatile unsigned long X = {};
    float targSpd  = {};
    float currSpd  = {};
    int dX = {};
    int pwm = {};
    const ShieldPinout& GetPinout() const noexcept {
        return pinout;
    }
private:
    void termsReset() noexcept;
    void PID() noexcept;
    
    const uint8_t num;
    /// @brief Params
    Callback cb;
    const ShieldPinout pinout = {};
    MotorParams params;   
    float xCoeff;
    float yCoeff;
    /// @brief Non Const States
    float ddist    = {};
    float lastSpd  = {};
    unsigned long lastMillis = {};
    float dTime = {};
    /// @brief PID States
    float interTerm = {};
    float lastError = {};
    /// @brief Encoder states
    unsigned long lastX = {};
    bool stopped = {};
    bool enabled = {true};
};

namespace detail {

static Motor* all;

template<size_t I>
void doRead() {
    all[I].X += digitalRead(all[I].GetPinout().encoderB) == HIGH ? 1 : -1;
}

template<size_t count, size_t...Is>
Motor* make(IndexSeq<Is...>) {
    static Motor::Callback cbs[count] = {doRead<Is>...};
    static Motor motors[count] = {Motor(Is, cbs[Is])...};
    return all = motors;
}

}

template<size_t count>
Motor* Make() {
    return detail::make<count>(MakeSeq<count>());
}
