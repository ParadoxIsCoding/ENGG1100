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
  WinchesPayout,
  WinchesRetrieve,
  LeftWinchPayout,
  LeftWinchRetrieve,
  RightWinchPayout,
  RightWinchRetrieve,
  Joystick,
};

class MotorController {
 public:
  struct MotorPins {
    uint8_t a;
    uint8_t b;
  };

  void begin();
  void apply(Motion motion);
  void applyJoystick(int8_t xPercent, int8_t yPercent);
  void stop();
  Motion motion() const;
  int8_t joystickX() const;
  int8_t joystickY() const;

 private:
  void drive(const MotorPins& motor, int16_t powerPercent);
  void logMotion(Motion motion) const;

  Motion motion_ = Motion::Stopped;
  int8_t joystickX_ = 0;
  int8_t joystickY_ = 0;
};

const char* motionName(Motion motion);
bool parseMotion(const String& value, Motion& motion);
