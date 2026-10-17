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
// PWM applied to one DRV8871 input at a time (the other held LOW). 20 kHz is
// above audible range; 8-bit duty is well within the ESP32 LEDC limit there.
constexpr uint16_t kMotorPwmFrequencyHz = 20000;
constexpr uint8_t kMotorPwmResolutionBits = 8;

// kDefaultSpeedPercent is what every command starts at; kMinSpeedPercent /
// kMaxSpeedPercent bound the /api/speed control's range.
constexpr uint8_t kMinSpeedPercent = 20;
constexpr uint8_t kDefaultSpeedPercent = 80;
constexpr uint8_t kMaxSpeedPercent = 100;

// Fixed speed used only by the calibration tab's
// hold-to-spin test. Independent of the drive speed slider so identifying a
// motor never happens at whatever speed drive was last left at. 15% was too
// little duty to start an N20 through a DRV8871 in fast-decay (coasting)
// PWM, so the test spin did nothing; 80% matches the default drive speed.
constexpr uint8_t kCalibrationSpeedPercent = 80;

#if BOARD_S3
// ------------------------------------------------------------------------
// ESP32-S3 N16R8 GPIO map, for the VCC-GND Studio YD-ESP32-S3 (silkscreen
// "YD-ESP32-23-2022-V1.3"), which follows the ESP32-S3-DevKitC-1 pinout.
// Every pin below is on the same header row (3V3, RST, 4, 5, 6, 7, 15, 16,
// 17, 18, 8, 3, 46, 9, ...), so the whole harness lands on one side.
//
// Deliberately avoided: strapping pins 0, 3, 45, 46; GPIO 19/20 (native
// USB D-/D+); GPIO 43/44 (UART0, the "COM" USB-C port used for flashing and
// the serial monitor); GPIO 26-32 (internal flash) and 33-37 (octal PSRAM,
// N16R8 in qio_opi mode); GPIO 48 (onboard RGB LED). GPIO 22-25 do not
// exist on the S3.
//
// As on the standard ESP32, these are physical wiring slots; which corner
// each drives is set from the calibration tab. The S3 has exactly 8 LEDC
// channels, which the 8 DRV8871 inputs use (see motor_controller.cpp).
// ------------------------------------------------------------------------
constexpr uint8_t kMotor1PinA = 4;   // DRV8871 #1, IN1
constexpr uint8_t kMotor1PinB = 5;   // DRV8871 #1, IN2
constexpr uint8_t kMotor2PinA = 6;   // DRV8871 #2, IN1
constexpr uint8_t kMotor2PinB = 7;   // DRV8871 #2, IN2
constexpr uint8_t kMotor3PinA = 15;  // DRV8871 #3, IN1
constexpr uint8_t kMotor3PinB = 16;  // DRV8871 #3, IN2
constexpr uint8_t kMotor4PinA = 17;  // DRV8871 #4, IN1
constexpr uint8_t kMotor4PinB = 18;  // DRV8871 #4, IN2

constexpr uint8_t kI2cSda = 8;  // GY-521 SDA
constexpr uint8_t kI2cScl = 9;  // GY-521 SCL
#else
// ------------------------------------------------------------------------
// Standard ESP32 (ESP32-D0WD-V3, esp32dev board definition) GPIO map.
// This matches the bench wiring exactly. Do not use GPIO 6-11: they are
// tied to the module's internal flash interface.
//
// These four motor slots are physical wiring positions, not corners — which
// corner each slot actually drives (and whether it needs to spin inverted)
// is a runtime mapping assigned from the web UI's calibration tab and
// stored in flash (see MotorController::loadCalibrationFromStorage). Swapping
// which winch is plugged into which DRV8871 no longer needs a firmware
// change, just re-running calibration.
//
// Each motor has its own single-channel DRV8871 module. "PinA" goes to that
// module's IN1 and "PinB" to its IN2; the module's OUT1/OUT2 go to the motor.
// ------------------------------------------------------------------------
constexpr uint8_t kMotor1PinA = 13;  // DRV8871 #1, IN1
constexpr uint8_t kMotor1PinB = 27;  // DRV8871 #1, IN2 (moved from GPIO 14,
                                     // which pulses during boot)
constexpr uint8_t kMotor2PinA = 16;  // DRV8871 #2, IN1
constexpr uint8_t kMotor2PinB = 17;  // DRV8871 #2, IN2
constexpr uint8_t kMotor3PinA = 18;  // DRV8871 #3, IN1
constexpr uint8_t kMotor3PinB = 19;  // DRV8871 #3, IN2
constexpr uint8_t kMotor4PinA = 32;  // DRV8871 #4, IN1 (moved from GPIO 25)
constexpr uint8_t kMotor4PinB = 33;  // DRV8871 #4, IN2 (moved from GPIO 26)

constexpr uint8_t kI2cSda = 21;  // GY-521 SDA
constexpr uint8_t kI2cScl = 22;  // GY-521 SCL
#endif

// GY-521 (MPU6050) tilt sensor, address 0x68 (AD0 tied low / floating).
// Set to false to skip I2C/sensor startup entirely; hardware mode still
// runs with manual motor control only.
constexpr bool kAttitudeSensorEnabled = true;

}  // namespace Config
