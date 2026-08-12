#pragma once

#include <Arduino.h>

class AttitudeSensor {
 public:
  void begin();
  void update();
  bool setLevel();
  float pitchDegrees() const;
  float rollDegrees() const;
  bool connected() const;
  bool demoMode() const;

 private:
  bool connectSensor();
  bool readRegister(uint8_t reg, uint8_t& value);
  bool writeRegister(uint8_t reg, uint8_t value);
  bool readAcceleration(float& xG, float& yG, float& zG);
  void updateDemo();

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
};
