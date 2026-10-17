#pragma once

#include <Arduino.h>

// Driver for a GY-521 MPU6050 board wired to I2C (Config::kI2cSda/kI2cScl).
// Only the accelerometer is used to compute pitch/roll; the gyroscope is
// left uninitialised, matching the "monitoring only, no auto-levelling"
// scope for this stage of the project.
//
// All I2C traffic runs on the sensor's own FreeRTOS task (started by begin()).
// When the bus is stuck or the sensor is faulty, each ESP-IDF I2C transaction
// blocks for about 1 s regardless of Wire's timeout, so the sensor must never
// be polled from the web server / main loop. The public getters and
// setLevel() only read or write the cached state under a mutex and never
// touch the bus.
class AttitudeSensor {
 public:
  void begin();
  bool setLevel();
  float pitchDegrees() const;
  float rollDegrees() const;
  const char* verticalMotionName() const;
  bool connected() const;
  bool demoMode() const;

 private:
  static void taskEntry(void* self);
  void update();
  bool connectSensor();
  bool readRegister(uint8_t reg, uint8_t& value);
  bool writeRegister(uint8_t reg, uint8_t value);
  bool readAcceleration(float& xG, float& yG, float& zG);
  bool updateVerticalMotion(float xG, float yG, float zG, uint32_t now);
  void updateDemo();
  void resetLevelLocked();

  enum class VerticalMotion : uint8_t { Steady, Rising, Falling };

  SemaphoreHandle_t stateMutex_ = nullptr;
  bool levelPending_ = true;  // Zero pitch/roll on the first sample after boot.
  uint8_t address_ = 0;
  bool connected_ = false;
  bool filterInitialised_ = false;
  uint8_t consecutiveReadFailures_ = 0;
  uint32_t lastProbeMs_ = 0;
  float filteredXG_ = 0.0f;
  float filteredYG_ = 0.0f;
  float filteredZG_ = 1.0f;
  float rawPitchDegrees_ = 0.0f;
  float rawRollDegrees_ = 0.0f;
  float pitchOffsetDegrees_ = 0.0f;
  float rollOffsetDegrees_ = 0.0f;
  float gravityXG_ = 0.0f;
  float gravityYG_ = 0.0f;
  float gravityZG_ = 1.0f;
  float filteredVerticalAccelerationG_ = 0.0f;
  uint32_t motionDisplayUntilMs_ = 0;
  uint32_t motionRearmAtMs_ = 0;
  bool motionDetectorArmed_ = false;
  VerticalMotion verticalMotion_ = VerticalMotion::Steady;
};
