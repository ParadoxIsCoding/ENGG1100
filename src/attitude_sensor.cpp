#include "attitude_sensor.h"

#include <math.h>

namespace {

// Temporary source used until the physical IMU driver is connected.
constexpr bool kUseSmoothDemoMode = true;

}  // namespace

void AttitudeSensor::begin() {
  // REAL IMU CONNECTION POINT:
  // Initialise Wire on Config::kI2cSda/Config::kI2cScl and the IMU here.
  update();
  setLevel();
  Serial.println("[attitude] smooth demo mode; real IMU not connected");
}

void AttitudeSensor::update() {
  if (kUseSmoothDemoMode) {
    const float seconds = static_cast<float>(millis()) / 1000.0f;
    rawPitchDegrees_ = 8.0f * sinf(seconds * 0.55f) +
                       3.5f * sinf(seconds * 0.19f);
    rawRollDegrees_ = 12.0f * sinf(seconds * 0.38f + 0.8f);
    return;
  }

  // REAL IMU CONNECTION POINT:
  // Read/filter the IMU here, then assign its output as follows:
  // rawPitchDegrees_ = filteredImuPitchDegrees;
  // rawRollDegrees_ = filteredImuRollDegrees;
}

void AttitudeSensor::setLevel() {
  update();
  pitchOffsetDegrees_ = rawPitchDegrees_;
  rollOffsetDegrees_ = rawRollDegrees_;
}

float AttitudeSensor::pitchDegrees() const {
  return rawPitchDegrees_ - pitchOffsetDegrees_;
}

float AttitudeSensor::rollDegrees() const {
  return rawRollDegrees_ - rollOffsetDegrees_;
}

bool AttitudeSensor::demoMode() const { return kUseSmoothDemoMode; }
