#pragma once

#include <Arduino.h>

class AttitudeSensor {
 public:
  void begin();
  void update();
  bool setLevel();
  float pitchDegrees() const;
  float rollDegrees() const;
  const char* verticalMotionName() const;
  bool connected() const;
  bool demoMode() const;

 private:
  bool connectSensor();
  bool readRegister(uint8_t reg, uint8_t& value);
  bool writeRegister(uint8_t reg, uint8_t value);
  bool readAcceleration(float& xG, float& yG, float& zG);
  void updateVerticalMotion(float xG, float yG, float zG, uint32_t now);
  void updateDemo();

  enum class VerticalMotion : uint8_t { Steady, Rising, Falling };

  uint8_t address_ = 0;
  bool connected_ = false;
  bool filterInitialised_ = false;
  uint8_t consecutiveReadFailures_ = 0;
  uint32_t lastReadMs_ = 0;
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
  float verticalVelocityMps_ = 0.0f;
  uint32_t lastMotionSampleMs_ = 0;
  VerticalMotion verticalMotion_ = VerticalMotion::Steady;
};
