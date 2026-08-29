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
  Calibration,
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

// A house corner. Which physical motor slot (see MotorController::MotorPins)
// actually drives each corner is a runtime mapping, not a fixed pin — see
// MotorController::applyCalibration.
enum class Corner : uint8_t {
  FrontLeft,
  FrontRight,
  RearLeft,
  RearRight,
};

class MotorController {
 public:
  struct MotorPins {
    uint8_t a;
    uint8_t b;
  };

  // Which corner one physical motor slot drives, and whether it needs to be
  // driven inverted to reel the right way. Assigned from the calibration tab.
  struct SlotCalibration {
    Corner corner;
    bool inverted;
  };

  static constexpr uint8_t kSlotCount = 4;

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

  // Calibration: spins exactly one physical motor slot (0..kSlotCount-1) at
  // the fixed Config::kCalibrationSpeedPercent, bypassing the corner mapping
  // entirely, so the slot can be visually identified. Call repeatedly
  // (hold-to-run) the same way as apply(); a call to stop() halts it.
  void calibrationSpin(uint8_t slot, bool payout);
  int8_t calibrationSlot() const;   // -1 when no calibration spin is active
  bool calibrationPayout() const;   // only meaningful when calibrationSlot() >= 0

  SlotCalibration slotCalibration(uint8_t slot) const;

  // Applies a new slot->corner/inverted mapping immediately and persists it
  // to flash. assignments must cover each Corner exactly once; returns false
  // (leaving the current mapping untouched) otherwise.
  bool applyCalibration(const SlotCalibration (&assignments)[kSlotCount]);
  // Restores the compiled-in default mapping and persists it.
  void resetCalibration();

 private:
  void drive(uint8_t slot, int16_t powerPercent);
  void driveCorner(Corner corner, int16_t powerPercent);
  void stopAllChannels();
  void logMotion(Motion motion) const;
  int8_t slotIndexForCorner(Corner corner) const;
  void loadCalibrationFromStorage();
  void persistCalibration() const;
  void loadSpeedFromStorage();
  void persistSpeed() const;

  Motion motion_ = Motion::Stopped;
  int8_t joystickX_ = 0;
  int8_t joystickY_ = 0;
  uint8_t speedPercent_ = 0;
  int16_t frontLeftPower_ = 0;
  int16_t frontRightPower_ = 0;
  int16_t rearLeftPower_ = 0;
  int16_t rearRightPower_ = 0;
  SlotCalibration slots_[kSlotCount];
  int8_t calibrationSlot_ = -1;
  bool calibrationPayout_ = false;
};

const char* motionName(Motion motion);
bool parseMotion(const String& value, Motion& motion);
const char* cornerName(Corner corner);
bool parseCorner(const String& value, Corner& corner);
