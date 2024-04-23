#pragma once

#include <stdint.h>
#include <stddef.h>
#include <utility>
#include <string.h>
#include <Arduino.h>

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
    using Callback = void(void*);

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
private:
    inline void termsReset() noexcept;
    void PID() noexcept;
    
    const uint8_t num;
    /// @brief Params
    Callback cb;
    const ShieldPinout pinout;
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

    template<int number>
    static void motor_cb() noexcept {
        getMotor(number)->X += (digitalRead(getMotor(number)->pinout.encoderB) == HIGH)? 1 : -1;
    }
};

namespace detail {

template<int count, size_t...Is>
Motor* make(Motor::Callback* cbs, std::index_sequence<Is...>) {
    static Motor* all;
    static Motor motors[count] = {Motor{cbs[Is] = []{
        all[Is].X += digitalRead(all[Is].pinout.encoderB) == HIGH ? 1 : -1;
    }}, ...};
    return all = motors;
}

}

template<int count>
Motor* Make() {
    static Motor::Callback cbs[count];
    return detail::make(cbs, std::make_index_sequence<count>());
}