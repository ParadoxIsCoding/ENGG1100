#include "attitude_sensor.h"

#include <Wire.h>
#include <math.h>

#include "config.h"

namespace {

// MPU6050 register map (RA_WHO_AM_I, RA_PWR_MGMT_1, RA_ACCEL_CONFIG,
// RA_ACCEL_XOUT_H). AD0 low (or floating, as wired here) gives 0x68; AD0
// high gives 0x69, tried as a fallback in case a board's AD0 is strapped.
constexpr uint8_t kWhoAmIRegister = 0x75;
constexpr uint8_t kExpectedWhoAmI = 0x68;
constexpr uint8_t kPwrMgmt1Register = 0x6B;
constexpr uint8_t kAccelConfigRegister = 0x1C;
constexpr uint8_t kAccelXoutHighRegister = 0x3B;
constexpr uint8_t kCandidateAddresses[] = {0x68, 0x69};
// LSB/g at the +-2g full-scale range selected in connectSensor().
constexpr float kAccelSensitivityLsbPerG = 16384.0f;
constexpr uint32_t kReadIntervalMs = 10;
// A failed probe of a stuck bus itself takes ~1 s, so don't retry constantly.
constexpr uint32_t kReconnectIntervalMs = 3000;
constexpr uint32_t kTaskStackBytes = 4096;
constexpr UBaseType_t kTaskPriority = 1;
// The Arduino loop (web server) runs on core 1; keep sensor I/O off it.
constexpr BaseType_t kTaskCore = 0;
constexpr float kLowPassAlpha = 0.35f;
constexpr float kGravityTrackingAlpha = 0.01f;
constexpr float kVerticalAccelerationAlpha = 0.40f;
constexpr float kMotionTriggerG = 0.008f;
constexpr float kGravityUpdateLimitG = 0.020f;
constexpr uint32_t kMotionDisplayMs = 1200;
constexpr uint32_t kInitialArmDelayMs = 500;
constexpr uint32_t kMotionRearmDelayMs = 350;
constexpr float kRadiansToDegrees = 57.2957795f;

bool deadlineReached(uint32_t now, uint32_t deadline) {
  return static_cast<int32_t>(now - deadline) >= 0;
}

class StateLock {
 public:
  explicit StateLock(SemaphoreHandle_t mutex) : mutex_(mutex) {
    if (mutex_ != nullptr) xSemaphoreTake(mutex_, portMAX_DELAY);
  }
  ~StateLock() {
    if (mutex_ != nullptr) xSemaphoreGive(mutex_);
  }
  StateLock(const StateLock&) = delete;
  StateLock& operator=(const StateLock&) = delete;

 private:
  SemaphoreHandle_t mutex_;
};

}  // namespace

void AttitudeSensor::begin() {
  if (!Config::kAttitudeSensorEnabled) {
    connected_ = false;
    Serial.println("[attitude] MPU6050 disabled in Config");
    return;
  }

  stateMutex_ = xSemaphoreCreateMutex();
  if (stateMutex_ == nullptr ||
      xTaskCreatePinnedToCore(taskEntry, "attitude", kTaskStackBytes, this,
                              kTaskPriority, nullptr, kTaskCore) != pdPASS) {
    Serial.println("[attitude] ERROR: could not start sensor task");
    return;
  }
  Serial.println("[attitude] probing MPU6050 in the background");
}

void AttitudeSensor::taskEntry(void* self) {
  auto* sensor = static_cast<AttitudeSensor*>(self);
  Wire.begin(Config::kI2cSda, Config::kI2cScl);
  Wire.setClock(400000);
  const bool found = sensor->connectSensor();
  {
    StateLock lock(sensor->stateMutex_);
    sensor->connected_ = found;
    sensor->lastProbeMs_ = millis();
  }
  if (!found) {
#if TEST_MODE
    Serial.println("[attitude] MPU6050 not found; using test-mode demo");
#else
    Serial.println("[attitude] ERROR: MPU6050 not found at 0x68 or 0x69");
#endif
  }
  while (true) {
    sensor->update();
    vTaskDelay(pdMS_TO_TICKS(kReadIntervalMs));
  }
}

// Sensor task only. I2C runs without the state mutex held, so a slow or
// stuck bus never blocks the getters used by the web server.
void AttitudeSensor::update() {
  const uint32_t now = millis();
  if (!connected_) {
    if (now - lastProbeMs_ >= kReconnectIntervalMs) {
      const bool found = connectSensor();
      StateLock lock(stateMutex_);
      lastProbeMs_ = millis();
      connected_ = found;
    }
    if (connected_) {
      Serial.printf("[attitude] MPU6050 reconnected at 0x%02X\n", address_);
      return;
    }
#if TEST_MODE
    StateLock lock(stateMutex_);
    updateDemo();
#endif
    return;
  }

  float xG = 0.0f;
  float yG = 0.0f;
  float zG = 0.0f;
  if (!readAcceleration(xG, yG, zG)) {
    if (++consecutiveReadFailures_ >= 3) {
      {
        StateLock lock(stateMutex_);
        connected_ = false;
        filterInitialised_ = false;
        lastProbeMs_ = now;
      }
      Serial.println("[attitude] ERROR: MPU6050 stopped responding");
    }
    return;
  }
  consecutiveReadFailures_ = 0;

  bool motionDetected = false;
  {
    StateLock lock(stateMutex_);
    if (!filterInitialised_) {
      filteredXG_ = xG;
      filteredYG_ = yG;
      filteredZG_ = zG;
      gravityXG_ = xG;
      gravityYG_ = yG;
      gravityZG_ = zG;
      filteredVerticalAccelerationG_ = 0.0f;
      verticalMotion_ = VerticalMotion::Steady;
      motionDisplayUntilMs_ = 0;
      motionRearmAtMs_ = now + kInitialArmDelayMs;
      motionDetectorArmed_ = false;
      filterInitialised_ = true;
    } else {
      motionDetected = updateVerticalMotion(xG, yG, zG, now);
      filteredXG_ += kLowPassAlpha * (xG - filteredXG_);
      filteredYG_ += kLowPassAlpha * (yG - filteredYG_);
      filteredZG_ += kLowPassAlpha * (zG - filteredZG_);
    }

    rawPitchDegrees_ =
        atan2f(filteredXG_,
               sqrtf(filteredYG_ * filteredYG_ + filteredZG_ * filteredZG_)) *
        kRadiansToDegrees;
    rawRollDegrees_ = atan2f(filteredYG_, filteredZG_) * kRadiansToDegrees;

    if (levelPending_) {
      resetLevelLocked();
      levelPending_ = false;
    }
  }
  if (motionDetected) {
    Serial.printf("[attitude] vertical motion: %s\n", verticalMotionName());
  }
}

void AttitudeSensor::resetLevelLocked() {
  pitchOffsetDegrees_ = rawPitchDegrees_;
  rollOffsetDegrees_ = rawRollDegrees_;
  gravityXG_ = filteredXG_;
  gravityYG_ = filteredYG_;
  gravityZG_ = filteredZG_;
  filteredVerticalAccelerationG_ = 0.0f;
  verticalMotion_ = VerticalMotion::Steady;
  motionDisplayUntilMs_ = 0;
  motionRearmAtMs_ = millis() + kInitialArmDelayMs;
  motionDetectorArmed_ = false;
}

bool AttitudeSensor::setLevel() {
  StateLock lock(stateMutex_);
  if (!connected_ && !demoMode()) return false;
  resetLevelLocked();
  return true;
}

float AttitudeSensor::pitchDegrees() const {
  StateLock lock(stateMutex_);
  return rawPitchDegrees_ - pitchOffsetDegrees_;
}

float AttitudeSensor::rollDegrees() const {
  StateLock lock(stateMutex_);
  return rawRollDegrees_ - rollOffsetDegrees_;
}

// verticalMotion_ is a single byte, so reading it without the lock is safe.
const char* AttitudeSensor::verticalMotionName() const {
  switch (verticalMotion_) {
    case VerticalMotion::Rising:
      return "rising";
    case VerticalMotion::Falling:
      return "falling";
    case VerticalMotion::Steady:
    default:
      return "steady";
  }
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

    // Clear the sleep bit to wake the sensor, then select the +-2g
    // accelerometer full-scale range.
    if (!writeRegister(kPwrMgmt1Register, 0x00) ||
        !writeRegister(kAccelConfigRegister, 0x00)) {
      continue;
    }

    {
      StateLock lock(stateMutex_);
      filterInitialised_ = false;
    }
    consecutiveReadFailures_ = 0;
    Serial.printf("[attitude] MPU6050 connected at 0x%02X\n", address_);
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
  Wire.write(kAccelXoutHighRegister);
  if (Wire.endTransmission(false) != 0 ||
      Wire.requestFrom(static_cast<int>(address_), 6) != 6) {
    return false;
  }

  // MPU6050 accelerometer output is a full signed 16-bit value (no shift
  // needed, unlike a 12-bit-in-16 part).
  const int16_t rawX = static_cast<int16_t>(
      (static_cast<uint16_t>(Wire.read()) << 8) | Wire.read());
  const int16_t rawY = static_cast<int16_t>(
      (static_cast<uint16_t>(Wire.read()) << 8) | Wire.read());
  const int16_t rawZ = static_cast<int16_t>(
      (static_cast<uint16_t>(Wire.read()) << 8) | Wire.read());
  xG = static_cast<float>(rawX) / kAccelSensitivityLsbPerG;
  yG = static_cast<float>(rawY) / kAccelSensitivityLsbPerG;
  zG = static_cast<float>(rawZ) / kAccelSensitivityLsbPerG;
  return true;
}

// Called with the state mutex held; returns true when a new rise/fall is
// detected so the caller can log it after releasing the lock.
bool AttitudeSensor::updateVerticalMotion(float xG, float yG, float zG,
                                          uint32_t now) {
  const float gravityMagnitude =
      sqrtf(gravityXG_ * gravityXG_ + gravityYG_ * gravityYG_ +
            gravityZG_ * gravityZG_);
  if (gravityMagnitude < 0.5f) {
    gravityXG_ = xG;
    gravityYG_ = yG;
    gravityZG_ = zG;
    motionRearmAtMs_ = now + kInitialArmDelayMs;
    motionDetectorArmed_ = false;
    return false;
  }

  const float inverseGravity = 1.0f / gravityMagnitude;
  const float verticalAccelerationG =
      ((xG - gravityXG_) * gravityXG_ +
       (yG - gravityYG_) * gravityYG_ +
       (zG - gravityZG_) * gravityZG_) *
      inverseGravity;
  filteredVerticalAccelerationG_ +=
      kVerticalAccelerationAlpha *
      (verticalAccelerationG - filteredVerticalAccelerationG_);

  // Slowly learn the stationary gravity vector so sensor bias cannot create a
  // permanent rise/fall indication. Motion is detected from the first impulse
  // and latched; the opposite impulse when the platform stops is ignored.
  if (fabsf(filteredVerticalAccelerationG_) < kGravityUpdateLimitG) {
    gravityXG_ += kGravityTrackingAlpha * (xG - gravityXG_);
    gravityYG_ += kGravityTrackingAlpha * (yG - gravityYG_);
    gravityZG_ += kGravityTrackingAlpha * (zG - gravityZG_);
  }

  if (!motionDetectorArmed_) {
    if (motionDisplayUntilMs_ != 0 &&
        deadlineReached(now, motionDisplayUntilMs_)) {
      motionDisplayUntilMs_ = 0;
      verticalMotion_ = VerticalMotion::Steady;
      motionRearmAtMs_ = now + kMotionRearmDelayMs;
    }
    if (motionDisplayUntilMs_ == 0 &&
        deadlineReached(now, motionRearmAtMs_)) {
      motionDetectorArmed_ = true;
    }
    return false;
  }

  if (filteredVerticalAccelerationG_ >= kMotionTriggerG) {
    verticalMotion_ = VerticalMotion::Rising;
  } else if (filteredVerticalAccelerationG_ <= -kMotionTriggerG) {
    verticalMotion_ = VerticalMotion::Falling;
  } else {
    verticalMotion_ = VerticalMotion::Steady;
    return false;
  }

  motionDetectorArmed_ = false;
  motionDisplayUntilMs_ = now + kMotionDisplayMs;
  return true;
}

void AttitudeSensor::updateDemo() {
  const float seconds = static_cast<float>(millis()) / 1000.0f;
  rawPitchDegrees_ = 8.0f * sinf(seconds * 0.55f) +
                     3.5f * sinf(seconds * 0.19f);
  rawRollDegrees_ = 12.0f * sinf(seconds * 0.38f + 0.8f);
  const float demoVerticalMotion = sinf(seconds * 0.7f);
  verticalMotion_ = demoVerticalMotion > 0.3f   ? VerticalMotion::Rising
                    : demoVerticalMotion < -0.3f ? VerticalMotion::Falling
                                                 : VerticalMotion::Steady;
}
