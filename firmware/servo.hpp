#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "FaBoPWM_PCA9685.h"

struct ServosSettings
{
    uint8_t channel = {};
    uint8_t speed = {};
    int minVal = {};
    int maxVal = 0;
};

class Servo
{
public:
    static inline FaBoPWM Shield;
    static inline bool wasinit = false;

    static void InitAll() {
        if (Shield.begin()) {
            for (int i = 0; i < 16; i++) {
                Shield.set_channel_value(i, 0);
            }
            Shield.init(300);
            wasinit = true;
        }
    }
    Servo(const ServosSettings &src = {}) : 
        settings(src)
    {
        if (init) {
            currVal = targetState = settings.minVal;
            Shield.set_channel_value(settings.channel, settings.minVal);
        }
    }
    void Command(uint8_t percents) {
        constrain(percents, 0, 100);
        targetState = pwmFromPercents(percents);
    }
    void Update() {
        if (abs(targetState - currVal) > settings.speed) {
            auto step = settings.speed;
            currVal += targetState > currVal ? step : -step;
        } else {
            currVal = targetState;
        }
        if (Disabled) {
            Shield.set_channel_value(settings.channel, 0);
        } else {
            Shield.set_channel_value(settings.channel, currVal);
        }
    }
    bool Disabled{false};
private:
    uint16_t pwmFromPercents(uint8_t percents) const {
        return settings.minVal + (settings.maxVal - settings.minVal) * ((float)percents / 100.f);
    }

    ServosSettings settings;
    uint16_t currVal;
    uint16_t targetState;
};
