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
// One N20 motor with a 3D-printed pulley sits at each corner and spools a
// fishing-line tether to a fixed anchor point in that corner's direction.
// Reeling a tether in pulls the house that way; the opposite corner(s) must
// pay out at the same time. Two dual-channel L9110S boards drive all four
// corner winches (one board per motor pair, channel A/B per motor).
constexpr uint8_t kFrontLeftWinchA = 13;
constexpr uint8_t kFrontLeftWinchB = 14;
constexpr uint8_t kFrontRightWinchA = 16;
constexpr uint8_t kFrontRightWinchB = 17;
constexpr uint8_t kRearLeftWinchA = 18;
constexpr uint8_t kRearLeftWinchB = 19;
constexpr uint8_t kRearRightWinchA = 25;
constexpr uint8_t kRearRightWinchB = 26;

// Jaycar XC3732 / Keyestudio KS0270 (MMA8452Q) I2C bus.
constexpr uint8_t kI2cSda = 21;
constexpr uint8_t kI2cScl = 22;

}  // namespace Config
