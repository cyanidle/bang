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
#include "MsgConfigServo.h"
#include "MsgConfigMotor.h"
#include "MsgTest.h"

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
static void Update();
static void Handle(Msg& msg);

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

#define DESERIALIZE(T) [&]{T res; parse_##T(&res, msg.body, msg.size); return res;}()

static void Handle(Msg& msg) {
  switch (msg.type) {
  case MsgPid_Type: {
    auto pid = DESERIALIZE(MsgPid);
    break;
  }
  case MsgServo_Type: {
    auto servo = DESERIALIZE(MsgServo);
    if (servo.servo >= MAX_SERVOS) {
        return;
    }
    GetServo(servo.servo).Command(servo.pos);
    break;
  }
  case MsgMove_Type: {
    auto move = DESERIALIZE(MsgMove);
    for (auto i = 0; i < MOTORS_COUNT; ++i) {
        motors[i].SpeedCallback(move.x, move.y, move.theta);
    }
    break;
  }
  case MsgConfigMotor_Type: {
    auto conf = DESERIALIZE(MsgConfigMotor);
    if (conf.num >= MOTORS_COUNT) {
        return;
    }
    MotorParams pars;
    pars.angleDegrees = conf.angleDegrees;
    pars.coeff = conf.coeff;
    pars.diffCoeff = conf.diffCoeff;
    pars.propCoeff = conf.propCoeff;
    pars.interCoeff = conf.interCoeff;
    pars.maxSpeed = conf.maxSpeed;
    pars.radius = conf.radius;
    pars.turnMaxSpeed = conf.turnMaxSpeed;
    pars.maxSpeed = conf.maxSpeed;
    pars.ticksPerRotation = conf.ticksPerRotation;
    motors[conf.num].SetParams(pars);
    ShieldPinout pinout;
    pinout.back = conf.back;
    pinout.fwd = conf.fwd;
    pinout.enable = conf.enable;
    pinout.encoderA = conf.encoderA;
    pinout.encoderB = conf.encoderB;
    motors[conf.num].SetPinout(pinout);
    break;
  }
  case MsgConfigServo_Type: {
    auto conf = DESERIALIZE(MsgConfigServo);
    if (conf.num >= MAX_SERVOS) {
        return;
    }
    ServosSettings pars;
    pars.channel = conf.channel;
    pars.maxVal = conf.maxVal;
    pars.minVal = conf.minVal;
    pars.speed = conf.speed;
    GetServo(conf.num) = {pars};
    break;
  }
  case MsgTest_Type: {
    auto test = DESERIALIZE(MsgTest);
    digitalWrite(LED_BUILTIN, test.led);
    break;
  }
  default: break;
  }
}
