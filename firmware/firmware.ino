#define BAUD_RATE 57600
#define MAX_SERVOS 15
#define MOTORS_COUNT 3
#define MSG_BUFFER 256

#include <Arduino.h>
#include "motor.hpp"
#include "Slip.hpp"
#include "kadyrovlcd.h"
#include "servo.hpp"

#include "MsgMove.h"
#include "MsgOdom.h"
#include "MsgPid.h"
#include "MsgServo.h"

const unsigned char OK_CODE = 11;
const unsigned char START_PIN_PRESENT = 12;
const unsigned char START_PIN_PULLED = 13;
const unsigned char BALL_PASSED = 14;

#define BALLS_FOR 3  // * MAIN_LOOP(10) = 40ms ball must be present to update state
#define BALLS_PIN 2

struct PinState {
  explicit PinState(int pin, int neededCount) : pin(pin), neededCount(neededCount) {}
  bool measure{ false };
  bool current{ false };
  int neededCount{};
  int currentCount{ 0 };
  int pin{};
};

bool debounce(PinState& state) {
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

static KadyrovLcd lcd(KadyrovLcd::Address::OLD);

struct Msg;
void Update();
void Handle(Msg& msg);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);
  Servo::InitAll();
  lcd.setup();
  Serial.begin(115200);
  Serial.setTimeout(5);
}

struct Msg {
  uint32_t id;
  uint16_t type;
  uint16_t flags;
  const char* body;
  size_t size;
};

bool ok = false;

static void handleFrame(char* begin, size_t size) {
  if (size < 8) {
    return;
  }
  Msg msg;
  memcpy(&msg.id, begin, 4);
  memcpy(&msg.type, begin + 4, 2);
  memcpy(&msg.flags, begin + 6, 2);
  msg.body = begin + 8;
  msg.size = size - 8;
  Handle(msg);
}

static Slip<handleFrame> slip;

void loop() {
  Update();
  slip.Read();
}

enum Flags {
  Request = 1,
  Ack = Request,
};


static void SendAck(uint32_t id) {
  char buff[8];
  uint16_t flags = Ack;
  memcpy(buff, &id, 4);
  memset(buff + 4, 0, 2);
  memcpy(buff + 6, &flags, 2);
  slip.Write(buff, sizeof(buff));
}

static void Handle(Msg& msg) {
  digitalWrite(LED_BUILTIN, bool(msg.type));

}

static Motor* motors = Make<MOTORS_COUNT>();
static Servo& GetServo(int num) {
    static Servo servos[MAX_SERVOS];
    return servos[num];
}

static void Update() {
  for (auto i = 0; i < MAX_SERVOS; ++i) {
    GetServo(i).Update();
  }
  for (auto i = 0; i < MOTORS_COUNT; ++i) {
    motors[i].Update();
  }
}