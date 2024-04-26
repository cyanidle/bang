#define BAUD_RATE 115200
#define MAX_SERVOS 15
#define MOTORS_COUNT 3
#define MSG_BUFFER 256
#define ODOM_DELAY_MS 30
#define ECHO_MSGS 1

#include <Arduino.h>
#include "motor.hpp"
#include "Slip.hpp"
#include "kadyrovlcd.h"
#include "servo.hpp"

#include "gen/MsgMove.h"
#include "gen/MsgOdom.h"
#include "gen/MsgPid.h"
#include "gen/MsgConfigMotor.h"
#include "gen/MsgConfigPinout.h"
#include "gen/MsgTest.h"
#include "gen/MsgEcho.h"
#include "gen/MsgReadPin.h"

template<typename T>
struct MsgTraits {
    static constexpr uint16_t Type = 0;
    static constexpr auto writer = nullptr;
    static constexpr auto reader = nullptr;
};
#define TRAITS_FOR(T) template<> struct MsgTraits<T> {\
static constexpr uint16_t Type = T##_Type;\
    static constexpr auto writer = dump_##T;\
    static constexpr auto reader = parse_##T;\
};

TRAITS_FOR(MsgMove)
TRAITS_FOR(MsgOdom)
TRAITS_FOR(MsgPid)
TRAITS_FOR(MsgConfigMotor)
TRAITS_FOR(MsgConfigPinout)
TRAITS_FOR(MsgTest)
TRAITS_FOR(MsgEcho)
TRAITS_FOR(MsgReadPin)

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

struct RawMsg;
static void Update();
static void Handle(RawMsg& msg);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);
  lcd.setup();
  Serial.begin(BAUD_RATE);
}

struct Throttle {
  unsigned long last = millis();
  unsigned long each;
  Throttle(unsigned long each) noexcept : each(each) {}
  bool operator()() noexcept {
    auto curr = millis();
    if (curr - last > each) {
        last = curr;
        return true;
    }
    return false;
  }
};

struct RawMsg {
  uint32_t id;
  uint16_t type;
  uint16_t flags;
  const char* body;
  size_t size;
};

static void handleFrame(char* begin, size_t size) {
  if (size < 8) {
    return;
  }
  RawMsg msg;
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

static Motor motors[MOTORS_COUNT] = {};

template<size_t I>
void MotorCb() {
  if (digitalRead(motors[I].GetPinout().encoderB) == HIGH) {
    motors[I].X++;
  } else {
    motors[I].X--;
  }
}

template<typename T, typename U>
static bool Deserialize(T& res, const U& msg) {
  return MsgTraits<T>::reader(&res, msg.body, msg.size) == msg.size;
}

template<typename T>
static void Send(const T& msg) {
  char buff[sizeof(T) + 8] = {};
  memcpy(buff + 4, &MsgTraits<T>::Type, 2);
  MsgTraits<T>::writer(&msg, buff + 8, sizeof(buff) - 8);
  slip.Write(buff, sizeof(buff));
}

static void Update() {
  static Throttle limit(ODOM_DELAY_MS);
  bool send = limit();
  for (auto i = 0; i < MOTORS_COUNT; ++i) {
    auto ddist = motors[i].Update();
    if (send) {
        MsgOdom msg = {};
        msg.num = i;
        msg.ddist_mm = ddist * 1000.f;
        Send(msg);
    }
  }
}

static void Handle(RawMsg& msg) {
#if ECHO_MSGS
  MsgEcho echo = {};
  echo.type = msg.type;
  echo.size = msg.size;
  Send(echo);
#endif
  switch (msg.type) {
  case MsgPid_Type: {
    MsgPid pid;
    if (!Deserialize(pid, msg)) {
      return;
    }
    break;
  }
  case MsgMove_Type: {
    MsgMove move;
    if (!Deserialize(move, msg)) {
      return;
    }
    float x = move.x/1000.f;
    float y = move.y/1000.f;
    float z = move.theta/1000.f;
    for (auto i = 0; i < MOTORS_COUNT; ++i) {
        motors[i].SpeedCallback(x, y, z);
    }
    break;
  }
  case MsgConfigMotor_Type: {
    MsgConfigMotor conf;
    if (!Deserialize(conf, msg)) {
      return;
    }
    if (conf.num >= MOTORS_COUNT) {
        return;
    }
    motors[conf.num].SetParams(conf);
    break;
  }
  case MsgReadPin_Type: {
    MsgReadPin pin;
    if (!Deserialize(pin, msg)) {
      return;
    }
    pinMode(pin.pin, pin.pullup ? INPUT_PULLUP : INPUT);
    pin.value = digitalRead(pin.pin);
    Send(pin); 
    break;
  }
  case MsgConfigPinout_Type: {
    MsgConfigPinout conf;
    if (!Deserialize(conf, msg)) {
      return;
    }
    if (conf.num >= MOTORS_COUNT) {
        return;
    }
    Motor::Callback cb = nullptr;
    switch (conf.num){
    case 0: cb = MotorCb<0>; break;
    case 1: cb = MotorCb<1>; break;
    case 2: cb = MotorCb<2>; break;
    default: return;
    }
    motors[conf.num].SetPinout(cb, conf);
    break;
  }
  case MsgTest_Type: {
    MsgTest test;
    if (!Deserialize(test, msg)) {
      return;
    }
    digitalWrite(LED_BUILTIN, test.led);
    Send(test);
    break;
  }
  default: break;
  }
}
