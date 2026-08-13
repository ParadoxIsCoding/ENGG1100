#include "motor_controller.h"

#include <math.h>

#include "config.h"

namespace {

// One corner winch per house corner. Positive power (see drive()) reels the
// tether in (retrieve); negative pays it out.
constexpr MotorController::MotorPins kFrontLeft{Config::kFrontLeftWinchA,
                                                Config::kFrontLeftWinchB};
constexpr MotorController::MotorPins kFrontRight{Config::kFrontRightWinchA,
                                                 Config::kFrontRightWinchB};
constexpr MotorController::MotorPins kRearLeft{Config::kRearLeftWinchA,
                                               Config::kRearLeftWinchB};
constexpr MotorController::MotorPins kRearRight{Config::kRearRightWinchA,
                                                Config::kRearRightWinchB};

#if !TEST_MODE
constexpr uint8_t kFrontLeftAChannel = 0;
constexpr uint8_t kFrontLeftBChannel = 1;
constexpr uint8_t kFrontRightAChannel = 2;
constexpr uint8_t kFrontRightBChannel = 3;
constexpr uint8_t kRearLeftAChannel = 4;
constexpr uint8_t kRearLeftBChannel = 5;
constexpr uint8_t kRearRightAChannel = 6;
constexpr uint8_t kRearRightBChannel = 7;

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

uint8_t channelForPin(uint8_t pin) {
  if (pin == kFrontLeft.a) return kFrontLeftAChannel;
  if (pin == kFrontLeft.b) return kFrontLeftBChannel;
  if (pin == kFrontRight.a) return kFrontRightAChannel;
  if (pin == kFrontRight.b) return kFrontRightBChannel;
  if (pin == kRearLeft.a) return kRearLeftAChannel;
  if (pin == kRearLeft.b) return kRearLeftBChannel;
  if (pin == kRearRight.a) return kRearRightAChannel;
  return kRearRightBChannel;
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

void MotorController::begin() {
#if TEST_MODE
  Serial.println("[motors] TEST_MODE: GPIO outputs are not enabled");
#else
  preparePwmOutput(kFrontLeft.a, kFrontLeftAChannel);
  preparePwmOutput(kFrontLeft.b, kFrontLeftBChannel);
  preparePwmOutput(kFrontRight.a, kFrontRightAChannel);
  preparePwmOutput(kFrontRight.b, kFrontRightBChannel);
  preparePwmOutput(kRearLeft.a, kRearLeftAChannel);
  preparePwmOutput(kRearLeft.b, kRearLeftBChannel);
  preparePwmOutput(kRearRight.a, kRearRightAChannel);
  preparePwmOutput(kRearRight.b, kRearRightBChannel);
  Serial.println("[motors] Hardware GPIO enabled; all outputs LOW");
#endif
  stop();
}

void MotorController::drive(const MotorPins& motor, int16_t powerPercent) {
#if TEST_MODE
  (void)motor;
  (void)powerPercent;
#else
  powerPercent = constrain(powerPercent, -100, 100);
  const uint32_t maxDuty = (1UL << Config::kMotorPwmResolutionBits) - 1;
  const uint32_t duty =
      static_cast<uint32_t>(abs(powerPercent)) * maxDuty / 100;
  ledcWrite(channelForPin(motor.a), powerPercent > 0 ? duty : 0);
  ledcWrite(channelForPin(motor.b), powerPercent < 0 ? duty : 0);
#endif
}

void MotorController::stopAllChannels() {
  // Stop all channels before changing direction to avoid shoot-through and
  // sudden opposite-direction transitions.
  drive(kFrontLeft, 0);
  drive(kFrontRight, 0);
  drive(kRearLeft, 0);
  drive(kRearRight, 0);
}

void MotorController::apply(Motion motion) {
  if (motion != Motion::Stopped && motion == motion_) {
    return;
  }

  stopAllChannels();
  joystickX_ = 0;
  joystickY_ = 0;

  // Every motion is a retrieve/payout mix across the four corner tethers.
  // Retrieving a corner pulls the house toward that corner's anchor; the
  // opposing corner(s) must pay out at the same time or the tethers fight
  // each other.
  switch (motion) {
    case Motion::Forward:
      drive(kFrontLeft, 100);
      drive(kFrontRight, 100);
      drive(kRearLeft, -100);
      drive(kRearRight, -100);
      break;
    case Motion::Reverse:
      drive(kFrontLeft, -100);
      drive(kFrontRight, -100);
      drive(kRearLeft, 100);
      drive(kRearRight, 100);
      break;
    case Motion::Left:
      drive(kFrontLeft, 100);
      drive(kRearLeft, 100);
      drive(kFrontRight, -100);
      drive(kRearRight, -100);
      break;
    case Motion::Right:
      drive(kFrontRight, 100);
      drive(kRearRight, 100);
      drive(kFrontLeft, -100);
      drive(kRearLeft, -100);
      break;
    case Motion::RotateLeft:
      drive(kFrontRight, 100);
      drive(kRearLeft, 100);
      drive(kFrontLeft, -100);
      drive(kRearRight, -100);
      break;
    case Motion::RotateRight:
      drive(kFrontLeft, 100);
      drive(kRearRight, 100);
      drive(kFrontRight, -100);
      drive(kRearLeft, -100);
      break;
    case Motion::AllPayout:
      drive(kFrontLeft, -Config::kWinchPowerPercent);
      drive(kFrontRight, -Config::kWinchPowerPercent);
      drive(kRearLeft, -Config::kWinchPowerPercent);
      drive(kRearRight, -Config::kWinchPowerPercent);
      break;
    case Motion::AllRetrieve:
      drive(kFrontLeft, Config::kWinchPowerPercent);
      drive(kFrontRight, Config::kWinchPowerPercent);
      drive(kRearLeft, Config::kWinchPowerPercent);
      drive(kRearRight, Config::kWinchPowerPercent);
      break;
    case Motion::FrontLeftPayout:
      drive(kFrontLeft, -Config::kWinchPowerPercent);
      break;
    case Motion::FrontLeftRetrieve:
      drive(kFrontLeft, Config::kWinchPowerPercent);
      break;
    case Motion::FrontRightPayout:
      drive(kFrontRight, -Config::kWinchPowerPercent);
      break;
    case Motion::FrontRightRetrieve:
      drive(kFrontRight, Config::kWinchPowerPercent);
      break;
    case Motion::RearLeftPayout:
      drive(kRearLeft, -Config::kWinchPowerPercent);
      break;
    case Motion::RearLeftRetrieve:
      drive(kRearLeft, Config::kWinchPowerPercent);
      break;
    case Motion::RearRightPayout:
      drive(kRearRight, -Config::kWinchPowerPercent);
      break;
    case Motion::RearRightRetrieve:
      drive(kRearRight, Config::kWinchPowerPercent);
      break;
    case Motion::Stopped:
    case Motion::Joystick:
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

  // Briefly stop every H-bridge input before applying a changed mix.
  stopAllChannels();
  drive(kFrontLeft, frontLeft);
  drive(kFrontRight, frontRight);
  drive(kRearLeft, rearLeft);
  drive(kRearRight, rearRight);

  joystickX_ = adjustedX;
  joystickY_ = adjustedY;
  motion_ = Motion::Joystick;
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

void MotorController::stop() { apply(Motion::Stopped); }

Motion MotorController::motion() const { return motion_; }

int8_t MotorController::joystickX() const { return joystickX_; }

int8_t MotorController::joystickY() const { return joystickY_; }

void MotorController::logMotion(Motion motion) const {
  Serial.printf("[motors] %s%s\n", motionName(motion),
#if TEST_MODE
                " (simulated)"
#else
                ""
#endif
  );
}
