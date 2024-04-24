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

static inline float toRadians(float degrees)
{
    return (degrees * 6.283f / 360.f);
}

class Motor
{
public:
    using Callback = void (*)();

    Motor(Callback cb) : cb(cb) {}
    void SetPinout(ShieldPinout const &params) {
        this->pinout = pinout;
        pinMode(pinout.encoderB, INPUT);
        pinMode(pinout.enable, OUTPUT);
        pinMode(pinout.fwd, OUTPUT);
        pinMode(pinout.back, OUTPUT);
        attachInterrupt(digitalPinToInterrupt(pinout.encoderA), cb, RISING);
    }
    void SetParams(const MotorParams &initStruct)
    {
        params = initStruct;
        xCoeff = cos(toRadians(initStruct.angleDegrees));
        yCoeff = sin(toRadians(initStruct.angleDegrees));
    }
    float Update()
    {
        if (!enabled) {
            dX = X = 0;
            digitalWrite(pinout.enable, HIGH);
            digitalWrite(pinout.fwd, HIGH);
            digitalWrite(pinout.back, HIGH);
            return 0;
        }
        auto current = millis();
        dTime = (current - lastMillis) / 1000.0;
        lastMillis = current;
        dX = static_cast<int>(X - lastX);
        lastX = X;
        ddist = dX * (params.radius / params.ticksPerRotation) * params.coeff;
        currSpd = ddist / dTime;
        if (stopped)
        {
            digitalWrite(pinout.enable, HIGH);
            digitalWrite(pinout.fwd, HIGH);
            digitalWrite(pinout.back, HIGH);
        }
        else
        {
            PID();
            if (pwm > 0)
            {
                analogWrite(pinout.enable, pwm);
                digitalWrite(pinout.fwd, HIGH);
                digitalWrite(pinout.back, LOW);
            }
            else
            {
                analogWrite(pinout.enable, -pwm); //////////pwm varies now from -255 to 255, so we use abs
                digitalWrite(pinout.fwd, LOW);
                digitalWrite(pinout.back, HIGH);
            }
        }
        return ddist;
    }

    void SpeedCallback(float x, float y, float turn) noexcept {
        if (!enabled) return;
        float spd = xCoeff * x * params.maxSpeed + yCoeff * y * params.maxSpeed;
        spd += turn * params.turnMaxSpeed;
        lastSpd = targSpd;
        //////IF speed is less than 1 cm/second then its not considered and PID terms are reset
        auto speedTooLow = -0.01 < spd && spd < 0.01;
        auto lastPositive = lastSpd > 0;
        auto newPositive = spd > 0;
        auto shouldResetTerms = newPositive != lastPositive;
        if (shouldResetTerms)
        {
            termsReset();
        }
        if (speedTooLow)
        {
            termsReset();
            targSpd = 0;
            lastSpd = 0;
            stopped = true;
            return;
        }
        stopped = false;
        targSpd = spd;
    }

    // TODO: Sepate fields into anon structs
    volatile unsigned long X = {};
    float targSpd = {};
    float currSpd = {};
    int dX = {};
    int pwm = {};
    const ShieldPinout &GetPinout() const noexcept
    {
        return pinout;
    }

private:
    void termsReset() noexcept {
        lastError = 0;
        interTerm = 0;
    }
    void PID() noexcept
    {
        float error = targSpd - currSpd;
        interTerm += dTime * error;
        pwm = error * params.propCoeff +
              interTerm * params.interCoeff -
              (error - lastError) / dTime * params.diffCoeff;
        interTerm = constrain(interTerm, -MAX_INTER_TERM, MAX_INTER_TERM);
        lastError = error;
        pwm = constrain(pwm, -MAX_PWM, MAX_PWM);
    }
    /// @brief Params
    Callback cb;
    const ShieldPinout pinout = {};
    MotorParams params;
    float xCoeff;
    float yCoeff;
    /// @brief Non Const States
    float ddist = {};
    float lastSpd = {};
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

namespace detail
{

static Motor *all;

template <size_t I>
void doRead()
{
    all[I].X += digitalRead(all[I].GetPinout().encoderB) == HIGH ? 1 : -1;
}

template <size_t count, size_t... Is>
Motor *make(IndexSeq<Is...>)
{
    static Motor::Callback cbs[count] = {doRead<Is>...};
    static Motor motors[count] = {Motor(cbs[Is])...};
    return all = motors;
}

}

template <size_t count>
Motor *Make()
{
    return detail::make<count>(MakeSeq<count>());
}
