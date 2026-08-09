#pragma once

#include <Arduino.h>

class AttitudeSensor {
 public:
  void begin();
  void update();
  void setLevel();
  float pitchDegrees() const;
  float rollDegrees() const;
  bool demoMode() const;

 private:
  // IMU PLACEHOLDERS: replace these two raw values in update() with the
  // filtered pitch and roll produced by the real IMU driver.
  float rawPitchDegrees_ = 0.0f;
  float rawRollDegrees_ = 0.0f;
  float pitchOffsetDegrees_ = 0.0f;
  float rollOffsetDegrees_ = 0.0f;
};
