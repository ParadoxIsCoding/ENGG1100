#pragma once

#include <Arduino.h>

#ifndef TEST_MODE
#define TEST_MODE 1
#endif

namespace Config {

constexpr char kAccessPointName[] = "ENGG1100-Lavender";
constexpr char kAccessPointPassword[] = "station1100";
constexpr uint16_t kHttpPort = 80;
constexpr uint32_t kDeadmanTimeoutMs = 600;
constexpr uint8_t kJoystickDeadZonePercent = 12;
constexpr uint16_t kMotorPwmFrequencyHz = 20000;
constexpr uint8_t kMotorPwmResolutionBits = 8;

// Change these only after checking the pinout for the exact ESP32-S3 board.
// Each L9110S motor channel uses two logic inputs.
constexpr uint8_t kFrontLeftA = 4;
constexpr uint8_t kFrontLeftB = 5;
constexpr uint8_t kFrontRightA = 6;
constexpr uint8_t kFrontRightB = 7;
constexpr uint8_t kRearLeftA = 15;
constexpr uint8_t kRearLeftB = 16;
constexpr uint8_t kRearRightA = 17;
constexpr uint8_t kRearRightB = 18;

// Reserved for the future MPU6050.
constexpr uint8_t kI2cSda = 8;
constexpr uint8_t kI2cScl = 9;

}  // namespace Config
