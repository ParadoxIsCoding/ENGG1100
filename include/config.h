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
constexpr uint32_t kMotorMutexTimeoutMs = 50;
constexpr uint8_t kJoystickDeadZonePercent = 12;
constexpr uint16_t kMotorPwmFrequencyHz = 20000;
constexpr uint8_t kMotorPwmResolutionBits = 8;
constexpr uint8_t kWinchPowerPercent = 65;

// ESP32-WROOM-32U GPIO map. Do not use GPIO 6-11: they are connected to flash.
// Each L9110S motor channel uses two logic inputs.
constexpr uint8_t kFrontLeftA = 13;
constexpr uint8_t kFrontLeftB = 14;
constexpr uint8_t kFrontRightA = 16;
constexpr uint8_t kFrontRightB = 17;
constexpr uint8_t kRearLeftA = 18;
constexpr uint8_t kRearLeftB = 19;
constexpr uint8_t kRearRightA = 25;
constexpr uint8_t kRearRightB = 26;

// Two reversible geared tether winches, each through one H-bridge channel.
constexpr uint8_t kLeftWinchA = 27;
constexpr uint8_t kLeftWinchB = 32;
constexpr uint8_t kRightWinchA = 23;
constexpr uint8_t kRightWinchB = 33;

// Jaycar XC3732 / Keyestudio KS0270 (MMA8452Q) I2C bus.
constexpr uint8_t kI2cSda = 21;
constexpr uint8_t kI2cScl = 22;

}  // namespace Config
