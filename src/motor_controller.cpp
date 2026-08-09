#include "motor_controller.h"

#include "config.h"

namespace {

constexpr MotorController::MotorPins kFrontLeft{Config::kFrontLeftA,
                                                Config::kFrontLeftB};
constexpr MotorController::MotorPins kFrontRight{Config::kFrontRightA,
                                                 Config::kFrontRightB};
constexpr MotorController::MotorPins kRearLeft{Config::kRearLeftA,
                                               Config::kRearLeftB};
constexpr MotorController::MotorPins kRearRight{Config::kRearRightA,
                                                Config::kRearRightB};

#if !TEST_MODE
void prepareStoppedOutput(uint8_t pin) {
  // Set the output latch LOW before enabling the output driver.
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
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
  prepareStoppedOutput(kFrontLeft.a);
  prepareStoppedOutput(kFrontLeft.b);
  prepareStoppedOutput(kFrontRight.a);
  prepareStoppedOutput(kFrontRight.b);
  prepareStoppedOutput(kRearLeft.a);
  prepareStoppedOutput(kRearLeft.b);
  prepareStoppedOutput(kRearRight.a);
  prepareStoppedOutput(kRearRight.b);
  Serial.println("[motors] Hardware GPIO enabled; all outputs LOW");
#endif
  stop();
}

void MotorController::drive(const MotorPins& motor, int8_t direction) {
#if TEST_MODE
  (void)motor;
  (void)direction;
#else
  if (direction > 0) {
    digitalWrite(motor.a, HIGH);
    digitalWrite(motor.b, LOW);
  } else if (direction < 0) {
    digitalWrite(motor.a, LOW);
    digitalWrite(motor.b, HIGH);
  } else {
    digitalWrite(motor.a, LOW);
    digitalWrite(motor.b, LOW);
  }
#endif
}

void MotorController::apply(Motion motion) {
  // Stop all channels before changing direction to avoid shoot-through and
  // sudden opposite-direction transitions.
  drive(kFrontLeft, 0);
  drive(kFrontRight, 0);
  drive(kRearLeft, 0);
  drive(kRearRight, 0);

  switch (motion) {
    case Motion::Forward:
      drive(kFrontLeft, 1);
      drive(kFrontRight, 1);
      drive(kRearLeft, 1);
      drive(kRearRight, 1);
      break;
    case Motion::Reverse:
      drive(kFrontLeft, -1);
      drive(kFrontRight, -1);
      drive(kRearLeft, -1);
      drive(kRearRight, -1);
      break;
    case Motion::Left:
      drive(kFrontLeft, -1);
      drive(kFrontRight, 1);
      drive(kRearLeft, -1);
      drive(kRearRight, 1);
      break;
    case Motion::Right:
      drive(kFrontLeft, 1);
      drive(kFrontRight, -1);
      drive(kRearLeft, 1);
      drive(kRearRight, -1);
      break;
    case Motion::RotateLeft:
      drive(kFrontLeft, -1);
      drive(kFrontRight, 1);
      drive(kRearLeft, 1);
      drive(kRearRight, -1);
      break;
    case Motion::RotateRight:
      drive(kFrontLeft, 1);
      drive(kFrontRight, -1);
      drive(kRearLeft, -1);
      drive(kRearRight, 1);
      break;
    case Motion::Stopped:
      break;
  }

  motion_ = motion;
  logMotion(motion);
}

void MotorController::stop() { apply(Motion::Stopped); }

Motion MotorController::motion() const { return motion_; }

void MotorController::logMotion(Motion motion) const {
  Serial.printf("[motors] %s%s\n", motionName(motion),
#if TEST_MODE
                " (simulated)"
#else
                ""
#endif
  );
}
