#define BAUD_RATE 57600
#define MAX_SERVOS 15
#define MOTORS_COUNT 3
#define MSG_BUFFER 256

#include <Arduino.h>
#include "motor.hpp"
#include "Slip.hpp"
#include "kadyrovlcd.h"
#include "servo.hpp"

#include "gen/MsgMove.h"
#include "gen/MsgOdom.h"
#include "gen/MsgPid.h"
#include "gen/MsgServo.h"
#include "gen/MsgConfigServo.h"
#include "gen/MsgConfigMotor.h"
#include "gen/MsgTest.h"

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
TRAITS_FOR(MsgServo)
TRAITS_FOR(MsgConfigServo)
TRAITS_FOR(MsgConfigMotor)
TRAITS_FOR(MsgTest)

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
  Servo::InitAll();
  lcd.setup();
  Serial.begin(115200);
  Serial.setTimeout(5);
}

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

template<typename T, typename U>
T Deserialize(const U& msg) {
  T res;
  MsgTraits<T>::reader(&res, msg.body, msg.size);
  return res;
}

template<typename T>
void Send(const T& msg) {
  char buff[sizeof(T) + 8] = {};
  memcpy(buff + 4, &MsgTraits<T>::Type, 2);
  MsgTraits<T>::writer(&msg, buff + 8, sizeof(buff) - 8);
  slip.Write(buff, sizeof(buff));
}

static void Handle(RawMsg& msg) {
  switch (msg.type) {
  case MsgPid_Type: {
    auto pid = Deserialize<MsgPid>(msg);
    break;
  }
  case MsgServo_Type: {
    auto servo = Deserialize<MsgServo>(msg);
    if (servo.servo >= MAX_SERVOS) {
        return;
    }
    GetServo(servo.servo).Command(servo.pos);
    break;
  }
  case MsgMove_Type: {
    auto move = Deserialize<MsgMove>(msg);
    for (auto i = 0; i < MOTORS_COUNT; ++i) {
        motors[i].SpeedCallback(move.x, move.y, move.theta);
    }
    break;
  }
  case MsgConfigMotor_Type: {
    auto conf = Deserialize<MsgConfigMotor>(msg);
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
    auto conf = Deserialize<MsgConfigServo>(msg);
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
    auto test = Deserialize<MsgTest>(msg);
    digitalWrite(LED_BUILTIN, test.led);
    Send(test);
    break;
  }
  default: break;
  }
}
