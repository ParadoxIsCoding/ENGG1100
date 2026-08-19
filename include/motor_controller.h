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
  Joystick,
  AllPayout,
  AllRetrieve,
  FrontLeftPayout,
  FrontLeftRetrieve,
  FrontRightPayout,
  FrontRightRetrieve,
  RearLeftPayout,
  RearLeftRetrieve,
  RearRightPayout,
  RearRightRetrieve,
};

class MotorController {
 public:
  struct MotorPins {
    uint8_t a;
    uint8_t b;
    bool inverted;
  };

  void begin();
  void apply(Motion motion);
  void applyJoystick(int8_t xPercent, int8_t yPercent);
  void stop();
  Motion motion() const;
  int8_t joystickX() const;
  int8_t joystickY() const;
  void setSpeedPercent(uint8_t percent);
  uint8_t speedPercent() const;
  int8_t frontLeftPowerPercent() const;
  int8_t frontRightPowerPercent() const;
  int8_t rearLeftPowerPercent() const;
  int8_t rearRightPowerPercent() const;

 private:
  void drive(const MotorPins& motor, int16_t powerPercent);
  void recordCommandedPower(const MotorPins& motor, int16_t powerPercent);
  void stopAllChannels();
  void logMotion(Motion motion) const;

  Motion motion_ = Motion::Stopped;
  int8_t joystickX_ = 0;
  int8_t joystickY_ = 0;
  uint8_t speedPercent_ = 0;
  int16_t frontLeftPower_ = 0;
  int16_t frontRightPower_ = 0;
  int16_t rearLeftPower_ = 0;
  int16_t rearRightPower_ = 0;
};

const char* motionName(Motion motion);
bool parseMotion(const String& value, Motion& motion);
