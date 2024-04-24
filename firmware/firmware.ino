#define BAUD_RATE 57600
#define MAX_SERVOS 15
#define MOTORS_COUNT 3

#include <Arduino.h>
#include "Slip.hpp"
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

void test() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);
  test();
  //Servo::InitAll();
  //lcd.setup();
  Serial.begin(57600);
  Serial.setTimeout(5);
}

struct Msg {
  uint32_t id;
  uint16_t type;
  uint16_t flags;
  const char* body;
  size_t size;
};

void loop() {
  test();
  Update();
  char buff[256];
  if (!Serial.readBytesUntil(SLIP::END, buff, sizeof(buff))) {
    return;
  }
  auto frame = SLIP::DeEscape(buff, buff + sizeof(buff));
  if (!frame) {
    return;
  }
  auto size = size_t(frame.end - frame.begin);
  if (size < 8) {
    return;
  }
  Msg msg;
  memcpy(&msg.id, frame.begin, 4);
  memcpy(&msg.type, frame.begin + 4, 2);
  memcpy(&msg.flags, frame.begin + 6, 2);
  msg.body = frame.begin + 8;
  msg.size = size - 8;
  Handle(msg);
}

//static Motor* motors = Make<MOTORS_COUNT>();
static Servo servos[MAX_SERVOS]{};

enum Flags {
  Request = 1,
  Ack = Request,
};

static void WriteSlip(const char* data, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    char ch = data[i];
    switch (ch) {
    case SLIP::END: {
      Serial.write(SLIP::ESC);
      Serial.write(SLIP::EscapedEnd);
      break;
    }
    case SLIP::ESC: {
      Serial.write(SLIP::ESC);
      Serial.write(SLIP::EscapedEsc);
      break;
    }
    default: {
      Serial.write(ch);
    }
    }
  }
  Serial.write(SLIP::END);
}

static void SendAck(uint32_t id) {
  char buff[8];
  uint16_t flags = Ack;
  memcpy(buff, &id, 4);
  memset(buff + 4, 0, 2);
  memcpy(buff + 6, &flags, 2);
  WriteSlip(buff, sizeof(buff));
}

static void Handle(Msg& msg) {
  digitalWrite(LED_BUILTIN, bool(msg.type));

}

static void Update() {
  for (auto i = 0; i < MAX_SERVOS; ++i) {
    servos[i].Update();
  }
  for (auto i = 0; i < MOTORS_COUNT; ++i) {
    //motors[i].Update();
  }
}