#pragma once

#include <Arduino.h>

enum class Motion : uint8_t {
  Stopped,
  Forward,
  Reverse,
  Left,
  Right,
  RotateLeft,
  RotateRight,
};

class MotorController {
 public:
  struct MotorPins {
    uint8_t a;
    uint8_t b;
  };

  void begin();
  void apply(Motion motion);
  void stop();
  Motion motion() const;

 private:
  void drive(const MotorPins& motor, int8_t direction);
  void logMotion(Motion motion) const;

  Motion motion_ = Motion::Stopped;
};

const char* motionName(Motion motion);
bool parseMotion(const String& value, Motion& motion);
