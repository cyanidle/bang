#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

struct ServosSettings
{
    uint8_t num = {};
    uint8_t channel = {};
    uint8_t speed = {};
    int minVal = {};
    int maxVal = 0;
    uint8_t startPercents = {};
};

class Servo
{
public:
    static void InitAll();
    Servo(const ServosSettings &src = {});
    void Command(uint8_t percents);
    void Update();
    bool Disabled{false};
private:
    uint16_t pwmFromPercents(uint8_t percents) const;

    ServosSettings settings;
    uint16_t currVal;
    uint16_t targetState;
};
