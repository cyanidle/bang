#define BAUD_RATE 57600
#define MAX_SERVOS 15
#define MOTORS_COUNT 3

#include <Arduino.h>
#include "motor.h"
#include "kadyrovlcd.h"
#include "servo.h"

#include "MsgMove.h"
#include "MsgOdom.h"
#include "MsgPid.h"
#include "MsgServo.h"

const unsigned char OK_CODE = 11;
const unsigned char START_PIN_PRESENT = 12;
const unsigned char START_PIN_PULLED = 13;
const unsigned char BALL_PASSED = 14;

#define BALLS_FOR 3 // * MAIN_LOOP(10) = 40ms ball must be present to update state
#define BALLS_PIN 2

struct PinState
{
  explicit PinState(int pin, int neededCount) : 
    pin(pin), neededCount(neededCount) 
  {}
  bool measure{false};
  bool current{false};
  int neededCount{};
  int currentCount{0};
  int pin{};
};

bool debounce(PinState& state)
{
  state.measure = digitalRead(state.pin);
  if (state.measure == state.current) {
    state.currentCount = 0;
  } else {
    state.currentCount++;
  }
  if (state.currentCount > state.neededCount) {
    state.currentCount = state.neededCount;
    state.current = state.measure;
  }
  return state.current;
}

static Motor* motors = Make<MOTORS_COUNT>();
static KadyrovLcd lcd(KadyrovLcd::Address::OLD);
static Servo servos[MAX_SERVOS]{};

void setup()
{
  lcd.setup();
}

void loop()
{
}
