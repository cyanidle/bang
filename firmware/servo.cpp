#include "servo.h"
#include "FaBoPWM_PCA9685.h"

static FaBoPWM Shield;
static bool wasSetup = false;

Servo::Servo(const ServosSettings &src) :
    settings(src),
    currVal(pwmFromPercents(src.startPercents)),
    targetState(pwmFromPercents(src.startPercents))
{
    if (!wasSetup) {
        if (Shield.begin()) {
            for (int i = 0; i < 16; i++) {
                Shield.set_channel_value(i, 0);
            }
            Shield.init(300);
        } else {
            //TODO
        }
        wasSetup = true;
    }

    Shield.set_channel_value(settings.channel, settings.minVal);
}

uint16_t Servo::pwmFromPercents(uint8_t percents) const
{
    return settings.minVal + (settings.maxVal - settings.minVal) * ((float) percents / 100.f);
}

void Servo::Update()
{
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

void Servo::Command(uint8_t fill)
{
    constrain(fill, 0, 100);
    targetState = pwmFromPercents(fill);
}