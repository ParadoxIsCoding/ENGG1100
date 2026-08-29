#include "motor_controller.h"

#include <math.h>

#include <Preferences.h>

#include "config.h"

namespace {

// Physical wiring only. Which corner each slot drives, and its invert
// flag, live in MotorController::slots_ (runtime, calibrated from the web
// UI) rather than here.
constexpr MotorController::MotorPins kSlotPins[MotorController::kSlotCount] = {
    {Config::kMotor1PinA, Config::kMotor1PinB},
    {Config::kMotor2PinA, Config::kMotor2PinB},
    {Config::kMotor3PinA, Config::kMotor3PinB},
    {Config::kMotor4PinA, Config::kMotor4PinB},
};

// Matches the wiring the project has always assumed: slot 1 = front-left,
// slot 2 = rear-left, slot 3 = front-right, slot 4 = rear-right, none
// inverted. Used until a saved calibration is found in flash, and restored
// by MotorController::resetCalibration.
constexpr MotorController::SlotCalibration kDefaultSlotCalibration
    [MotorController::kSlotCount] = {
        {Corner::FrontLeft, false},
        {Corner::RearLeft, false},
        {Corner::FrontRight, false},
        {Corner::RearRight, false},
};

constexpr char kPrefsNamespace[] = "motorcal";
constexpr char kPrefsKey[] = "slots";

// Separate namespace for user-adjustable settings (currently just drive
// speed) that persist across reboots, distinct from the slot calibration.
constexpr char kSettingsNamespace[] = "settings";
constexpr char kSpeedKey[] = "speed";

#if !TEST_MODE
constexpr uint8_t kSlotAChannel[MotorController::kSlotCount] = {0, 2, 4, 6};
constexpr uint8_t kSlotBChannel[MotorController::kSlotCount] = {1, 3, 5, 7};

void prepareStoppedOutput(uint8_t pin) {
  // Set the output latch LOW before enabling the output driver.
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
}

void preparePwmOutput(uint8_t pin, uint8_t channel) {
  prepareStoppedOutput(pin);
  ledcSetup(channel, Config::kMotorPwmFrequencyHz,
            Config::kMotorPwmResolutionBits);
  ledcAttachPin(pin, channel);
  ledcWrite(channel, 0);
}
#endif

}  // namespace

const char* motionName(Motion motion) {
  switch (motion) {
    case Motion::Forward:
      return "forward";
    case Motion::Reverse:
      return "reverse";
    case Motion::Left:
      return "left";
    case Motion::Right:
      return "right";
    case Motion::RotateLeft:
      return "rotate-left";
    case Motion::RotateRight:
      return "rotate-right";
    case Motion::AllPayout:
      return "all-payout";
    case Motion::AllRetrieve:
      return "all-retrieve";
    case Motion::FrontLeftPayout:
      return "front-left-payout";
    case Motion::FrontLeftRetrieve:
      return "front-left-retrieve";
    case Motion::FrontRightPayout:
      return "front-right-payout";
    case Motion::FrontRightRetrieve:
      return "front-right-retrieve";
    case Motion::RearLeftPayout:
      return "rear-left-payout";
    case Motion::RearLeftRetrieve:
      return "rear-left-retrieve";
    case Motion::RearRightPayout:
      return "rear-right-payout";
    case Motion::RearRightRetrieve:
      return "rear-right-retrieve";
    case Motion::Joystick:
      return "joystick";
    case Motion::Calibration:
      return "calibrating";
    case Motion::Stopped:
    default:
      return "stopped";
  }
}

bool parseMotion(const String& value, Motion& motion) {
  if (value == "forward") {
    motion = Motion::Forward;
  } else if (value == "reverse") {
    motion = Motion::Reverse;
  } else if (value == "left") {
    motion = Motion::Left;
  } else if (value == "right") {
    motion = Motion::Right;
  } else if (value == "rotate-left") {
    motion = Motion::RotateLeft;
  } else if (value == "rotate-right") {
    motion = Motion::RotateRight;
  } else if (value == "all-payout") {
    motion = Motion::AllPayout;
  } else if (value == "all-retrieve") {
    motion = Motion::AllRetrieve;
  } else if (value == "front-left-payout") {
    motion = Motion::FrontLeftPayout;
  } else if (value == "front-left-retrieve") {
    motion = Motion::FrontLeftRetrieve;
  } else if (value == "front-right-payout") {
    motion = Motion::FrontRightPayout;
  } else if (value == "front-right-retrieve") {
    motion = Motion::FrontRightRetrieve;
  } else if (value == "rear-left-payout") {
    motion = Motion::RearLeftPayout;
  } else if (value == "rear-left-retrieve") {
    motion = Motion::RearLeftRetrieve;
  } else if (value == "rear-right-payout") {
    motion = Motion::RearRightPayout;
  } else if (value == "rear-right-retrieve") {
    motion = Motion::RearRightRetrieve;
  } else if (value == "stop") {
    motion = Motion::Stopped;
  } else {
    return false;
  }
  return true;
}

const char* cornerName(Corner corner) {
  switch (corner) {
    case Corner::FrontLeft:
      return "front-left";
    case Corner::FrontRight:
      return "front-right";
    case Corner::RearLeft:
      return "rear-left";
    case Corner::RearRight:
    default:
      return "rear-right";
  }
}

bool parseCorner(const String& value, Corner& corner) {
  if (value == "front-left") {
    corner = Corner::FrontLeft;
  } else if (value == "front-right") {
    corner = Corner::FrontRight;
  } else if (value == "rear-left") {
    corner = Corner::RearLeft;
  } else if (value == "rear-right") {
    corner = Corner::RearRight;
  } else {
    return false;
  }
  return true;
}

void MotorController::begin() {
  speedPercent_ = Config::kDefaultSpeedPercent;
  for (uint8_t i = 0; i < kSlotCount; ++i) slots_[i] = kDefaultSlotCalibration[i];
  loadCalibrationFromStorage();
  loadSpeedFromStorage();
#if TEST_MODE
  Serial.println("[motors] TEST_MODE: GPIO outputs are not enabled");
#else
  for (uint8_t i = 0; i < kSlotCount; ++i) {
    preparePwmOutput(kSlotPins[i].a, kSlotAChannel[i]);
    preparePwmOutput(kSlotPins[i].b, kSlotBChannel[i]);
  }
  Serial.println("[motors] Hardware GPIO enabled; all outputs LOW");
#endif
  stop();
}

void MotorController::drive(uint8_t slot, int16_t powerPercent) {
  // Polarity is applied here, once, from the calibrated invert flag, so a
  // reversed motor is fixed from the calibration tab rather than the
  // movement or joystick math.
  if (slot >= kSlotCount) return;
  if (slots_[slot].inverted) powerPercent = static_cast<int16_t>(-powerPercent);
  powerPercent = constrain(powerPercent, -100, 100);
#if TEST_MODE
  (void)powerPercent;
#else
  const uint32_t maxDuty = (1UL << Config::kMotorPwmResolutionBits) - 1;
  const uint32_t duty =
      static_cast<uint32_t>(abs(powerPercent)) * maxDuty / 100;
  ledcWrite(kSlotAChannel[slot], powerPercent > 0 ? duty : 0);
  ledcWrite(kSlotBChannel[slot], powerPercent < 0 ? duty : 0);
#endif
}

int8_t MotorController::slotIndexForCorner(Corner corner) const {
  for (uint8_t i = 0; i < kSlotCount; ++i) {
    if (slots_[i].corner == corner) return static_cast<int8_t>(i);
  }
  return -1;
}

void MotorController::driveCorner(Corner corner, int16_t powerPercent) {
  powerPercent = constrain(powerPercent, static_cast<int16_t>(-100),
                            static_cast<int16_t>(100));
  switch (corner) {
    case Corner::FrontLeft:
      frontLeftPower_ = powerPercent;
      break;
    case Corner::FrontRight:
      frontRightPower_ = powerPercent;
      break;
    case Corner::RearLeft:
      rearLeftPower_ = powerPercent;
      break;
    case Corner::RearRight:
      rearRightPower_ = powerPercent;
      break;
  }
  const int8_t slot = slotIndexForCorner(corner);
  if (slot >= 0) drive(static_cast<uint8_t>(slot), powerPercent);
}

void MotorController::stopAllChannels() {
  // Stop every physical slot directly (not via the corner mapping) before
  // changing direction, to avoid shoot-through and sudden opposite-direction
  // transitions even if the mapping is mid-calibration.
  for (uint8_t i = 0; i < kSlotCount; ++i) drive(i, 0);
  frontLeftPower_ = 0;
  frontRightPower_ = 0;
  rearLeftPower_ = 0;
  rearRightPower_ = 0;
}

void MotorController::apply(Motion motion) {
  if (motion != Motion::Stopped && motion == motion_) {
    return;
  }

  stopAllChannels();
  joystickX_ = 0;
  joystickY_ = 0;
  calibrationSlot_ = -1;

  // Every motion drives at the current commanded speed (see setSpeedPercent),
  // which defaults low and is capped by Config::kMaxSpeedPercent for initial
  // bench testing. Every motion is a retrieve/payout mix across the four
  // corner tethers: retrieving a corner pulls the house toward that corner's
  // anchor, and the opposing corner(s) must pay out at the same time or the
  // tethers fight each other.
  const int16_t p = speedPercent_;
  switch (motion) {
    case Motion::Forward:
      driveCorner(Corner::FrontLeft, p);
      driveCorner(Corner::FrontRight, p);
      driveCorner(Corner::RearLeft, -p);
      driveCorner(Corner::RearRight, -p);
      break;
    case Motion::Reverse:
      driveCorner(Corner::FrontLeft, -p);
      driveCorner(Corner::FrontRight, -p);
      driveCorner(Corner::RearLeft, p);
      driveCorner(Corner::RearRight, p);
      break;
    case Motion::Left:
      driveCorner(Corner::FrontLeft, p);
      driveCorner(Corner::RearLeft, p);
      driveCorner(Corner::FrontRight, -p);
      driveCorner(Corner::RearRight, -p);
      break;
    case Motion::Right:
      driveCorner(Corner::FrontRight, p);
      driveCorner(Corner::RearRight, p);
      driveCorner(Corner::FrontLeft, -p);
      driveCorner(Corner::RearLeft, -p);
      break;
    case Motion::RotateLeft:
      driveCorner(Corner::FrontRight, p);
      driveCorner(Corner::RearLeft, p);
      driveCorner(Corner::FrontLeft, -p);
      driveCorner(Corner::RearRight, -p);
      break;
    case Motion::RotateRight:
      driveCorner(Corner::FrontLeft, p);
      driveCorner(Corner::RearRight, p);
      driveCorner(Corner::FrontRight, -p);
      driveCorner(Corner::RearLeft, -p);
      break;
    case Motion::AllPayout:
      driveCorner(Corner::FrontLeft, -p);
      driveCorner(Corner::FrontRight, -p);
      driveCorner(Corner::RearLeft, -p);
      driveCorner(Corner::RearRight, -p);
      break;
    case Motion::AllRetrieve:
      driveCorner(Corner::FrontLeft, p);
      driveCorner(Corner::FrontRight, p);
      driveCorner(Corner::RearLeft, p);
      driveCorner(Corner::RearRight, p);
      break;
    case Motion::FrontLeftPayout:
      driveCorner(Corner::FrontLeft, -p);
      break;
    case Motion::FrontLeftRetrieve:
      driveCorner(Corner::FrontLeft, p);
      break;
    case Motion::FrontRightPayout:
      driveCorner(Corner::FrontRight, -p);
      break;
    case Motion::FrontRightRetrieve:
      driveCorner(Corner::FrontRight, p);
      break;
    case Motion::RearLeftPayout:
      driveCorner(Corner::RearLeft, -p);
      break;
    case Motion::RearLeftRetrieve:
      driveCorner(Corner::RearLeft, p);
      break;
    case Motion::RearRightPayout:
      driveCorner(Corner::RearRight, -p);
      break;
    case Motion::RearRightRetrieve:
      driveCorner(Corner::RearRight, p);
      break;
    case Motion::Stopped:
    case Motion::Joystick:
    case Motion::Calibration:
      break;
  }

  motion_ = motion;
  logMotion(motion);
}

void MotorController::applyJoystick(int8_t xPercent, int8_t yPercent) {
  xPercent = constrain(xPercent, -100, 100);
  yPercent = constrain(yPercent, -100, 100);

  const int16_t radiusSquared =
      static_cast<int16_t>(xPercent) * xPercent +
      static_cast<int16_t>(yPercent) * yPercent;
  const int16_t deadZoneSquared =
      Config::kJoystickDeadZonePercent * Config::kJoystickDeadZonePercent;
  if (radiusSquared <= deadZoneSquared) {
    stop();
    return;
  }

  const float radius = min(sqrtf(radiusSquared), 100.0f);
  const float scaledRadius =
      (radius - Config::kJoystickDeadZonePercent) * 100.0f /
      (100.0f - Config::kJoystickDeadZonePercent);
  const int8_t adjustedX = static_cast<int8_t>(
      lroundf(static_cast<float>(xPercent) / sqrtf(radiusSquared) *
              scaledRadius));
  const int8_t adjustedY = static_cast<int8_t>(
      lroundf(static_cast<float>(yPercent) / sqrtf(radiusSquared) *
              scaledRadius));

  if (motion_ == Motion::Joystick && joystickX_ == adjustedX &&
      joystickY_ == adjustedY) {
    return;
  }

  // Holonomic mix: each corner retrieves for the forward/left components of
  // the requested vector that point toward it, and pays out otherwise.
  int16_t frontLeft = static_cast<int16_t>(adjustedY) + adjustedX;
  int16_t frontRight = static_cast<int16_t>(adjustedY) - adjustedX;
  int16_t rearLeft = -static_cast<int16_t>(adjustedY) + adjustedX;
  int16_t rearRight = -static_cast<int16_t>(adjustedY) - adjustedX;
  const int16_t peak = max(max(abs(frontLeft), abs(frontRight)),
                            max(abs(rearLeft), abs(rearRight)));
  if (peak > 100) {
    frontLeft = frontLeft * 100 / peak;
    frontRight = frontRight * 100 / peak;
    rearLeft = rearLeft * 100 / peak;
    rearRight = rearRight * 100 / peak;
  }

  // Scale the normalised -100..100 mix down to the current commanded speed.
  const float speedScale = static_cast<float>(speedPercent_) / 100.0f;
  frontLeft = static_cast<int16_t>(lroundf(frontLeft * speedScale));
  frontRight = static_cast<int16_t>(lroundf(frontRight * speedScale));
  rearLeft = static_cast<int16_t>(lroundf(rearLeft * speedScale));
  rearRight = static_cast<int16_t>(lroundf(rearRight * speedScale));

  // Briefly stop every H-bridge input before applying a changed mix.
  stopAllChannels();
  driveCorner(Corner::FrontLeft, frontLeft);
  driveCorner(Corner::FrontRight, frontRight);
  driveCorner(Corner::RearLeft, rearLeft);
  driveCorner(Corner::RearRight, rearRight);

  joystickX_ = adjustedX;
  joystickY_ = adjustedY;
  motion_ = Motion::Joystick;
  calibrationSlot_ = -1;
  Serial.printf("[motors] joystick x=%d y=%d fl=%d fr=%d rl=%d rr=%d%s\n",
                adjustedX, adjustedY, frontLeft, frontRight, rearLeft,
                rearRight,
#if TEST_MODE
                " (simulated)"
#else
                ""
#endif
  );
}

void MotorController::calibrationSpin(uint8_t slot, bool payout) {
  if (slot >= kSlotCount) return;
  stopAllChannels();
  const int16_t power = payout
                             ? static_cast<int16_t>(-Config::kCalibrationSpeedPercent)
                             : static_cast<int16_t>(Config::kCalibrationSpeedPercent);
  drive(slot, power);
  motion_ = Motion::Calibration;
  joystickX_ = 0;
  joystickY_ = 0;
  calibrationSlot_ = static_cast<int8_t>(slot);
  calibrationPayout_ = payout;
  Serial.printf("[motors] calibration spin slot=%u %s%s\n", slot + 1,
                payout ? "payout" : "retrieve",
#if TEST_MODE
                " (simulated)"
#else
                ""
#endif
  );
}

int8_t MotorController::calibrationSlot() const { return calibrationSlot_; }

bool MotorController::calibrationPayout() const { return calibrationPayout_; }

MotorController::SlotCalibration MotorController::slotCalibration(
    uint8_t slot) const {
  if (slot >= kSlotCount) return {Corner::FrontLeft, false};
  return slots_[slot];
}

bool MotorController::applyCalibration(
    const SlotCalibration (&assignments)[kSlotCount]) {
  bool seen[4] = {false, false, false, false};
  for (uint8_t i = 0; i < kSlotCount; ++i) {
    const uint8_t cornerIndex = static_cast<uint8_t>(assignments[i].corner);
    if (cornerIndex >= 4 || seen[cornerIndex]) return false;
    seen[cornerIndex] = true;
  }

  stop();  // Motors are fully stopped before the slot->corner mapping moves.
  for (uint8_t i = 0; i < kSlotCount; ++i) slots_[i] = assignments[i];
  persistCalibration();
  Serial.println("[motors] calibration saved");
  return true;
}

void MotorController::resetCalibration() {
  stop();
  for (uint8_t i = 0; i < kSlotCount; ++i) slots_[i] = kDefaultSlotCalibration[i];
  persistCalibration();
  Serial.println("[motors] calibration reset to default");
}

void MotorController::loadCalibrationFromStorage() {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, /*readOnly=*/true)) return;
  uint8_t blob[kSlotCount * 2];
  if (prefs.getBytesLength(kPrefsKey) == sizeof(blob) &&
      prefs.getBytes(kPrefsKey, blob, sizeof(blob)) == sizeof(blob)) {
    SlotCalibration candidate[kSlotCount];
    bool seen[4] = {false, false, false, false};
    bool valid = true;
    for (uint8_t i = 0; i < kSlotCount && valid; ++i) {
      const uint8_t cornerIndex = blob[i * 2];
      if (cornerIndex >= 4 || seen[cornerIndex]) {
        valid = false;
        break;
      }
      seen[cornerIndex] = true;
      candidate[i].corner = static_cast<Corner>(cornerIndex);
      candidate[i].inverted = blob[i * 2 + 1] != 0;
    }
    if (valid) {
      for (uint8_t i = 0; i < kSlotCount; ++i) slots_[i] = candidate[i];
      Serial.println("[motors] loaded saved calibration from flash");
    } else {
      Serial.println("[motors] saved calibration was invalid; using default");
    }
  }
  prefs.end();
}

void MotorController::persistCalibration() const {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, /*readOnly=*/false)) {
    Serial.println("[motors] failed to open flash storage to save calibration");
    return;
  }
  uint8_t blob[kSlotCount * 2];
  for (uint8_t i = 0; i < kSlotCount; ++i) {
    blob[i * 2] = static_cast<uint8_t>(slots_[i].corner);
    blob[i * 2 + 1] = slots_[i].inverted ? 1 : 0;
  }
  prefs.putBytes(kPrefsKey, blob, sizeof(blob));
  prefs.end();
}

void MotorController::loadSpeedFromStorage() {
  Preferences prefs;
  if (!prefs.begin(kSettingsNamespace, /*readOnly=*/true)) return;
  if (prefs.isKey(kSpeedKey)) {
    const uint8_t saved = prefs.getUChar(kSpeedKey, speedPercent_);
    if (saved >= Config::kMinSpeedPercent && saved <= Config::kMaxSpeedPercent) {
      speedPercent_ = saved;
      Serial.printf("[motors] loaded saved speed from flash: %u%%\n", saved);
    }
  }
  prefs.end();
}

void MotorController::persistSpeed() const {
  Preferences prefs;
  if (!prefs.begin(kSettingsNamespace, /*readOnly=*/false)) {
    Serial.println("[motors] failed to open flash storage to save speed");
    return;
  }
  prefs.putUChar(kSpeedKey, speedPercent_);
  prefs.end();
}

void MotorController::stop() { apply(Motion::Stopped); }

Motion MotorController::motion() const { return motion_; }

int8_t MotorController::joystickX() const { return joystickX_; }

int8_t MotorController::joystickY() const { return joystickY_; }

void MotorController::setSpeedPercent(uint8_t percent) {
  const uint8_t clamped = static_cast<uint8_t>(
      constrain(percent, Config::kMinSpeedPercent, Config::kMaxSpeedPercent));
  if (clamped == speedPercent_) return;
  speedPercent_ = clamped;
  persistSpeed();
}

uint8_t MotorController::speedPercent() const { return speedPercent_; }

int8_t MotorController::frontLeftPowerPercent() const {
  return static_cast<int8_t>(frontLeftPower_);
}

int8_t MotorController::frontRightPowerPercent() const {
  return static_cast<int8_t>(frontRightPower_);
}

int8_t MotorController::rearLeftPowerPercent() const {
  return static_cast<int8_t>(rearLeftPower_);
}

int8_t MotorController::rearRightPowerPercent() const {
  return static_cast<int8_t>(rearRightPower_);
}

void MotorController::logMotion(Motion motion) const {
  Serial.printf("[motors] %s%s\n", motionName(motion),
#if TEST_MODE
                " (simulated)"
#else
                ""
#endif
  );
}
