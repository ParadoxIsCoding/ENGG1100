#pragma once

#include <Arduino.h>

#ifndef TEST_MODE
#define TEST_MODE 1
#endif

// Set by platformio.ini on the esp32-s3-* environments. Standard ESP32
// environments (esp32-test / esp32-hardware) leave this at 0.
#ifndef BOARD_S3
#define BOARD_S3 0
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

// First-power bench-test limits: an 8xAA pack with no fuse is powering the
// L9110S boards. kDefaultSpeedPercent is what every command starts at;
// kMaxSpeedPercent is the current ceiling for the /api/speed control. Raise
// kMaxSpeedPercent only after current draw and mechanical behaviour have
// been checked under load.
constexpr uint8_t kMinSpeedPercent = 5;
constexpr uint8_t kDefaultSpeedPercent = 20;
constexpr uint8_t kMaxSpeedPercent = 35;

#if BOARD_S3
// ------------------------------------------------------------------------
// Placeholder ESP32-S3 N16R8 GPIO map. This board is NOT physically wired
// yet; these numbers are carried over from the standard-ESP32 map only as a
// starting point. Before wiring the S3, re-check every pin against the
// actual module pinout: avoid strapping pins (0, 3, 45, 46) and, in
// qio_opi (octal PSRAM) mode as configured in platformio.ini, avoid
// GPIO 26-32 and 33-37, which the S3 module uses internally for flash/PSRAM.
// Do NOT assume this list is safe until it has been checked.
// ------------------------------------------------------------------------
constexpr uint8_t kFrontLeftWinchA = 13;
constexpr uint8_t kFrontLeftWinchB = 14;
constexpr uint8_t kRearLeftWinchA = 16;
constexpr uint8_t kRearLeftWinchB = 17;
constexpr uint8_t kFrontRightWinchA = 18;
constexpr uint8_t kFrontRightWinchB = 19;
constexpr uint8_t kRearRightWinchA = 25;
constexpr uint8_t kRearRightWinchB = 26;

constexpr uint8_t kI2cSda = 21;
constexpr uint8_t kI2cScl = 22;
#else
// ------------------------------------------------------------------------
// Standard ESP32 (ESP32-D0WD-V3, esp32dev board definition) GPIO map.
// This matches the bench wiring exactly. Do not use GPIO 6-11: they are
// tied to the module's internal flash interface.
//
// Driver 1: channel A = Front Left, channel B = Rear Left
// Driver 2: channel A = Front Right, channel B = Rear Right
// ------------------------------------------------------------------------
constexpr uint8_t kFrontLeftWinchA = 13;   // L9110S #1, A-IA
constexpr uint8_t kFrontLeftWinchB = 14;   // L9110S #1, A-IB
constexpr uint8_t kRearLeftWinchA = 16;    // L9110S #1, B-IA
constexpr uint8_t kRearLeftWinchB = 17;    // L9110S #1, B-IB
constexpr uint8_t kFrontRightWinchA = 18;  // L9110S #2, A-IA
constexpr uint8_t kFrontRightWinchB = 19;  // L9110S #2, A-IB
constexpr uint8_t kRearRightWinchA = 25;   // L9110S #2, B-IA
constexpr uint8_t kRearRightWinchB = 26;   // L9110S #2, B-IB

constexpr uint8_t kI2cSda = 21;  // GY-521 SDA
constexpr uint8_t kI2cScl = 22;  // GY-521 SCL
#endif

// Motor wire polarity has not been physically verified. If a motor reels
// the wrong way on first test, flip its flag here — never touch the
// movement/joystick math to compensate for a single reversed motor.
constexpr bool kFrontLeftInverted = false;
constexpr bool kFrontRightInverted = false;
constexpr bool kRearLeftInverted = false;
constexpr bool kRearRightInverted = false;

// GY-521 (MPU6050) tilt sensor, address 0x68 (AD0 tied low / floating).
// Set to false to skip I2C/sensor startup entirely; hardware mode still
// runs with manual motor control only.
constexpr bool kAttitudeSensorEnabled = true;

}  // namespace Config
