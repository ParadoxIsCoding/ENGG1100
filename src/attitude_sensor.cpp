#include "attitude_sensor.h"

#include <Wire.h>
#include <math.h>

#include "config.h"

namespace {

constexpr uint8_t kWhoAmIRegister = 0x0D;
constexpr uint8_t kExpectedWhoAmI = 0x2A;
constexpr uint8_t kOutXMsbRegister = 0x01;
constexpr uint8_t kXyzDataConfigRegister = 0x0E;
constexpr uint8_t kCtrlReg1 = 0x2A;
constexpr uint8_t kCtrlReg2 = 0x2B;
constexpr uint8_t kCandidateAddresses[] = {0x1D, 0x1C};
constexpr uint32_t kReadIntervalMs = 20;
constexpr uint32_t kReconnectIntervalMs = 1000;
constexpr float kLowPassAlpha = 0.18f;
constexpr float kRadiansToDegrees = 57.2957795f;

}  // namespace

void AttitudeSensor::begin() {
  Wire.begin(Config::kI2cSda, Config::kI2cScl);
  Wire.setClock(400000);
  connected_ = connectSensor();
  update();
  setLevel();

  if (!connected_) {
#if TEST_MODE
    Serial.println("[attitude] MMA8452Q not found; using test-mode demo");
#else
    Serial.println("[attitude] ERROR: MMA8452Q not found at 0x1C or 0x1D");
#endif
  }
}

void AttitudeSensor::update() {
  const uint32_t now = millis();
  if (!connected_) {
    if (now - lastProbeMs_ >= kReconnectIntervalMs) {
      lastProbeMs_ = now;
      connected_ = connectSensor();
      if (connected_) {
        Serial.printf("[attitude] MMA8452Q reconnected at 0x%02X\n", address_);
      }
    }
#if TEST_MODE
    updateDemo();
#endif
    return;
  }

  // Always take the first valid sample immediately so startup calibration does
  // not accidentally use the initial zero values.
  if (filterInitialised_ && now - lastReadMs_ < kReadIntervalMs) return;
  lastReadMs_ = now;

  float xG = 0.0f;
  float yG = 0.0f;
  float zG = 0.0f;
  if (!readAcceleration(xG, yG, zG)) {
    if (++consecutiveReadFailures_ >= 3) {
      connected_ = false;
      filterInitialised_ = false;
      lastProbeMs_ = now;
      Serial.println("[attitude] ERROR: MMA8452Q stopped responding");
    }
    return;
  }
  consecutiveReadFailures_ = 0;

  if (!filterInitialised_) {
    filteredXG_ = xG;
    filteredYG_ = yG;
    filteredZG_ = zG;
    filterInitialised_ = true;
  } else {
    filteredXG_ += kLowPassAlpha * (xG - filteredXG_);
    filteredYG_ += kLowPassAlpha * (yG - filteredYG_);
    filteredZG_ += kLowPassAlpha * (zG - filteredZG_);
  }

  rawPitchDegrees_ =
      atan2f(-filteredXG_,
             sqrtf(filteredYG_ * filteredYG_ + filteredZG_ * filteredZG_)) *
      kRadiansToDegrees;
  rawRollDegrees_ = atan2f(filteredYG_, filteredZG_) * kRadiansToDegrees;
}

bool AttitudeSensor::setLevel() {
  update();
  if (!connected_ && !demoMode()) return false;
  pitchOffsetDegrees_ = rawPitchDegrees_;
  rollOffsetDegrees_ = rawRollDegrees_;
  return true;
}

float AttitudeSensor::pitchDegrees() const {
  return rawPitchDegrees_ - pitchOffsetDegrees_;
}

float AttitudeSensor::rollDegrees() const {
  return rawRollDegrees_ - rollOffsetDegrees_;
}

bool AttitudeSensor::connected() const { return connected_; }

bool AttitudeSensor::demoMode() const {
#if TEST_MODE
  return !connected_;
#else
  return false;
#endif
}

bool AttitudeSensor::connectSensor() {
  for (const uint8_t candidate : kCandidateAddresses) {
    address_ = candidate;
    uint8_t whoAmI = 0;
    if (!readRegister(kWhoAmIRegister, whoAmI) ||
        whoAmI != kExpectedWhoAmI) {
      continue;
    }

    uint8_t ctrl1 = 0;
    if (!readRegister(kCtrlReg1, ctrl1) ||
        !writeRegister(kCtrlReg1, ctrl1 & ~0x01) ||
        !writeRegister(kXyzDataConfigRegister, 0x00) ||
        !writeRegister(kCtrlReg2, 0x02) ||
        !writeRegister(kCtrlReg1, 0x21)) {
      continue;
    }

    filterInitialised_ = false;
    consecutiveReadFailures_ = 0;
    lastReadMs_ = 0;
    Serial.printf("[attitude] MMA8452Q connected at 0x%02X\n", address_);
    return true;
  }
  address_ = 0;
  return false;
}

bool AttitudeSensor::readRegister(uint8_t reg, uint8_t& value) {
  if (address_ == 0) return false;
  Wire.beginTransmission(address_);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0 ||
      Wire.requestFrom(static_cast<int>(address_), 1) != 1) {
    return false;
  }
  value = Wire.read();
  return true;
}

bool AttitudeSensor::writeRegister(uint8_t reg, uint8_t value) {
  if (address_ == 0) return false;
  Wire.beginTransmission(address_);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool AttitudeSensor::readAcceleration(float& xG, float& yG, float& zG) {
  Wire.beginTransmission(address_);
  Wire.write(kOutXMsbRegister);
  if (Wire.endTransmission(false) != 0 ||
      Wire.requestFrom(static_cast<int>(address_), 6) != 6) {
    return false;
  }

  // The output is left-aligned signed 12-bit two's-complement. Convert to a
  // signed 16-bit value before shifting so negative acceleration is preserved.
  const int16_t rawX = static_cast<int16_t>(
                           (static_cast<uint16_t>(Wire.read()) << 8) |
                           Wire.read()) >>
                       4;
  const int16_t rawY = static_cast<int16_t>(
                           (static_cast<uint16_t>(Wire.read()) << 8) |
                           Wire.read()) >>
                       4;
  const int16_t rawZ = static_cast<int16_t>(
                           (static_cast<uint16_t>(Wire.read()) << 8) |
                           Wire.read()) >>
                       4;
  xG = static_cast<float>(rawX) / 1024.0f;
  yG = static_cast<float>(rawY) / 1024.0f;
  zG = static_cast<float>(rawZ) / 1024.0f;
  return true;
}

void AttitudeSensor::updateDemo() {
  const float seconds = static_cast<float>(millis()) / 1000.0f;
  rawPitchDegrees_ = 8.0f * sinf(seconds * 0.55f) +
                     3.5f * sinf(seconds * 0.19f);
  rawRollDegrees_ = 12.0f * sinf(seconds * 0.38f + 0.8f);
}
