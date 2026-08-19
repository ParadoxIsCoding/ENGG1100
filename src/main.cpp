#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <stdlib.h>

#include "attitude_sensor.h"
#include "config.h"
#include "motor_controller.h"
#include "web_page.h"

namespace {

WebServer server(Config::kHttpPort);
MotorController motors;
AttitudeSensor attitude;
SemaphoreHandle_t motorMutex = nullptr;
bool emergencyStopLatched = false;
volatile bool motionActive = false;
volatile uint32_t lastValidMotionMs = 0;
String lastCommandLabel = "none";
String lastSafetyReason = "none";

// Takes motorMutex for its scope. Check ok() before touching motor state;
// on timeout/missing mutex the lock is simply not held.
class MotorMutexGuard {
 public:
  MotorMutexGuard()
      : locked_(motorMutex != nullptr &&
                xSemaphoreTake(motorMutex,
                               pdMS_TO_TICKS(Config::kMotorMutexTimeoutMs)) ==
                    pdTRUE) {}
  ~MotorMutexGuard() {
    if (locked_) xSemaphoreGive(motorMutex);
  }
  MotorMutexGuard(const MotorMutexGuard&) = delete;
  MotorMutexGuard& operator=(const MotorMutexGuard&) = delete;

  bool ok() const { return locked_; }

 private:
  bool locked_;
};

Motion currentMotion() {
  MotorMutexGuard guard;
  return guard.ok() ? motors.motion() : Motion::Stopped;
}

void applyMotion(Motion motion) {
  MotorMutexGuard guard;
  if (!guard.ok()) {
    motionActive = false;
    return;
  }
  motors.apply(motion);
}

void applyJoystick(int8_t xPercent, int8_t yPercent) {
  MotorMutexGuard guard;
  if (!guard.ok()) {
    motionActive = false;
    return;
  }
  motors.applyJoystick(xPercent, yPercent);
}

int8_t currentJoystickAxis(bool xAxis) {
  MotorMutexGuard guard;
  if (!guard.ok()) return 0;
  return xAxis ? motors.joystickX() : motors.joystickY();
}

struct MotorPowers {
  int8_t frontLeft;
  int8_t frontRight;
  int8_t rearLeft;
  int8_t rearRight;
};

MotorPowers currentMotorPowers() {
  MotorMutexGuard guard;
  if (!guard.ok()) return {0, 0, 0, 0};
  return {motors.frontLeftPowerPercent(), motors.frontRightPowerPercent(),
          motors.rearLeftPowerPercent(), motors.rearRightPowerPercent()};
}

uint8_t currentSpeedPercent() {
  MotorMutexGuard guard;
  return guard.ok() ? motors.speedPercent() : 0;
}

String statusJson() {
  attitude.update();
  String json;
  json.reserve(320);
  json = F("{\"motion\":\"");
  json += motionName(currentMotion());
  json += F("\",\"estop\":");
  json += emergencyStopLatched ? F("true") : F("false");
  json += F(",\"deadZonePercent\":");
  json += static_cast<int>(Config::kJoystickDeadZonePercent);
  json += F(",\"x\":");
  json += static_cast<int>(currentJoystickAxis(true));
  json += F(",\"y\":");
  json += static_cast<int>(currentJoystickAxis(false));
  json += F(",\"testMode\":");
#if TEST_MODE
  json += F("true");
#else
  json += F("false");
#endif
  json += F(",\"pitch\":");
  json += String(attitude.pitchDegrees(), 1);
  json += F(",\"roll\":");
  json += String(attitude.rollDegrees(), 1);
  json += F(",\"verticalMotion\":\"");
  json += attitude.verticalMotionName();
  json += F("\"");
  json += F(",\"attitudeDemo\":");
  json += attitude.demoMode() ? F("true") : F("false");
  json += F(",\"attitudeConnected\":");
  json += attitude.connected() ? F("true") : F("false");
  json += F(",\"speedPercent\":");
  json += static_cast<int>(currentSpeedPercent());
  json += F(",\"minSpeedPercent\":");
  json += static_cast<int>(Config::kMinSpeedPercent);
  json += F(",\"maxSpeedPercent\":");
  json += static_cast<int>(Config::kMaxSpeedPercent);
  json += F(",\"uptimeMs\":");
  json += String(millis());
  json += F(",\"clients\":");
  json += static_cast<int>(WiFi.softAPgetStationNum());
  json += F(",\"lastCommand\":\"");
  json += lastCommandLabel;
  json += F("\",\"lastSafetyEvent\":\"");
  json += lastSafetyReason;
  json += F("\",\"deadmanActive\":");
  json += motionActive ? F("true") : F("false");
  {
    const MotorPowers powers = currentMotorPowers();
    json += F(",\"motorPower\":{\"frontLeft\":");
    json += static_cast<int>(powers.frontLeft);
    json += F(",\"frontRight\":");
    json += static_cast<int>(powers.frontRight);
    json += F(",\"rearLeft\":");
    json += static_cast<int>(powers.rearLeft);
    json += F(",\"rearRight\":");
    json += static_cast<int>(powers.rearRight);
    json += F("}");
  }
  json += F("}");
  return json;
}

bool parsePercent(const String& value, int8_t& result) {
  char* end = nullptr;
  const long parsed = strtol(value.c_str(), &end, 10);
  if (end == value.c_str() || *end != '\0' || parsed < -100 || parsed > 100) {
    return false;
  }
  result = static_cast<int8_t>(parsed);
  return true;
}

void sendJson(int code, const String& body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", body);
}

void stopForSafety(const char* reason) {
  lastSafetyReason = reason;
  MotorMutexGuard guard;
  if (!guard.ok()) return;
  const bool wasMoving = motors.motion() != Motion::Stopped;
  motors.stop();
  if (wasMoving) {
    Serial.printf("[safety] stop: %s\n", reason);
  }
}

void safetyTask(void*) {
  while (true) {
    if (motionActive &&
        millis() - lastValidMotionMs > Config::kDeadmanTimeoutMs) {
      motionActive = false;
      stopForSafety("dead-man timeout");
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void handleClientDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  (void)event;
  (void)info;
  motionActive = false;
  stopForSafety("Wi-Fi client disconnected");
}

void handleMove() {
  if (emergencyStopLatched) {
    sendJson(423, F("{\"error\":\"Emergency stop is latched\"}"));
    return;
  }

  if (!server.hasArg("direction")) {
    stopForSafety("missing direction");
    sendJson(400, F("{\"error\":\"Missing direction\"}"));
    return;
  }

  Motion requested = Motion::Stopped;
  if (!parseMotion(server.arg("direction"), requested) ||
      requested == Motion::Stopped) {
    stopForSafety("invalid direction");
    sendJson(400, F("{\"error\":\"Invalid direction\"}"));
    return;
  }

  lastCommandLabel = motionName(requested);
  lastValidMotionMs = millis();
  applyMotion(requested);
  motionActive = currentMotion() != Motion::Stopped;
  sendJson(200, statusJson());
}

void handleDrive() {
  if (emergencyStopLatched) {
    sendJson(423, F("{\"error\":\"Emergency stop is latched\"}"));
    return;
  }

  int8_t xPercent = 0;
  int8_t yPercent = 0;
  if (!server.hasArg("x") || !server.hasArg("y") ||
      !parsePercent(server.arg("x"), xPercent) ||
      !parsePercent(server.arg("y"), yPercent)) {
    motionActive = false;
    stopForSafety("invalid joystick vector");
    sendJson(400, F("{\"error\":\"X and Y must be integers from -100 to 100\"}"));
    return;
  }

  lastCommandLabel = "joystick";
  lastValidMotionMs = millis();
  applyJoystick(xPercent, yPercent);
  motionActive = currentMotion() != Motion::Stopped;
  sendJson(200, statusJson());
}

void handleStop() {
  lastCommandLabel = "stop";
  motionActive = false;
  stopForSafety("stop command");
  sendJson(200, statusJson());
}

void handleEmergencyStop() {
  lastCommandLabel = "estop";
  emergencyStopLatched = true;
  motionActive = false;
  stopForSafety("emergency stop");
  Serial.println("[safety] emergency stop latched");
  sendJson(200, statusJson());
}

void handleSpeed() {
  if (!server.hasArg("value")) {
    sendJson(400, F("{\"error\":\"Missing value\"}"));
    return;
  }
  int8_t percent = 0;
  if (!parsePercent(server.arg("value"), percent) || percent < 0) {
    sendJson(400, F("{\"error\":\"value must be 0-100\"}"));
    return;
  }
  {
    MotorMutexGuard guard;
    if (guard.ok()) motors.setSpeedPercent(static_cast<uint8_t>(percent));
  }
  Serial.printf("[motors] speed set to %u%%\n", currentSpeedPercent());
  sendJson(200, statusJson());
}

void handleClearEmergencyStop() {
  // Clearing the latch never starts motion; a new hold-to-run command is needed.
  lastCommandLabel = "estop-clear";
  motionActive = false;
  stopForSafety("clear emergency stop");
  emergencyStopLatched = false;
  Serial.println("[safety] emergency stop cleared; motors remain stopped");
  sendJson(200, statusJson());
}

void handleSetLevel() {
  if (!attitude.setLevel()) {
    sendJson(503, F("{\"error\":\"MPU6050 is not connected\"}"));
    return;
  }
  Serial.println("[attitude] current position set as level");
  sendJson(200, statusJson());
}

void configureServer() {
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html", kControlPage);
  });
  server.on("/api/status", HTTP_GET,
            []() { sendJson(200, statusJson()); });
  server.on("/api/move", HTTP_POST, handleMove);
  server.on("/api/drive", HTTP_POST, handleDrive);
  server.on("/api/stop", HTTP_POST, handleStop);
  server.on("/api/estop", HTTP_POST, handleEmergencyStop);
  server.on("/api/estop/clear", HTTP_POST, handleClearEmergencyStop);
  server.on("/api/speed", HTTP_POST, handleSpeed);
  server.on("/api/attitude/level", HTTP_POST, handleSetLevel);
  server.onNotFound([]() {
    stopForSafety("unknown request");
    sendJson(404, F("{\"error\":\"Not found\"}"));
  });
  server.begin();
}

void printPinAssignments() {
  Serial.println("Motor pin assignments (GPIO):");
  Serial.printf("  Front Left : A=%u B=%u%s\n", Config::kFrontLeftWinchA,
                Config::kFrontLeftWinchB,
                Config::kFrontLeftInverted ? " (inverted)" : "");
  Serial.printf("  Front Right: A=%u B=%u%s\n", Config::kFrontRightWinchA,
                Config::kFrontRightWinchB,
                Config::kFrontRightInverted ? " (inverted)" : "");
  Serial.printf("  Rear Left  : A=%u B=%u%s\n", Config::kRearLeftWinchA,
                Config::kRearLeftWinchB,
                Config::kRearLeftInverted ? " (inverted)" : "");
  Serial.printf("  Rear Right : A=%u B=%u%s\n", Config::kRearRightWinchA,
                Config::kRearRightWinchB,
                Config::kRearRightInverted ? " (inverted)" : "");
  Serial.printf("  I2C: SDA=%u SCL=%u\n", Config::kI2cSda, Config::kI2cScl);
}

}  // namespace

void setup() {
  // Required safe startup order:
  // 1. Serial. 2-4. Motor GPIO configured as outputs and forced LOW.
  // 5. Emergency-stop/failsafe state initialised. 6. I2C/MPU6050 probe.
  // 7-8. Wi-Fi AP and web server. All motor-control pins are LOW before any
  // of steps 6-8 run.
  Serial.begin(115200);

  motors.begin();  // Configures all 8 motor GPIOs as outputs, forces them
                    // LOW, and sets the internal motor state to STOPPED.

  motorMutex = xSemaphoreCreateMutex();
  if (motorMutex == nullptr ||
      xTaskCreate(safetyTask, "motor-safety", 2048, nullptr, 3, nullptr) !=
          pdPASS) {
    Serial.println("[fatal] safety task failed; controller will not start");
    motors.stop();
    return;
  }
  emergencyStopLatched = false;
  motionActive = false;
  lastValidMotionMs = 0;

  attitude.begin();  // I2C/MPU6050 probe; motor pins are already LOW.

  WiFi.onEvent(handleClientDisconnected,
               ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(Config::kAccessPointName,
                   Config::kAccessPointPassword)) {
    Serial.println("[fatal] Wi-Fi access point failed; motors remain stopped");
    return;
  }

  configureServer();

  Serial.println();
  Serial.println("ENGG1100 Station Keeper ready");
  Serial.printf("Firmware mode: %s\n",
                TEST_MODE ? "TEST (no motor hardware required)" : "HARDWARE");
  Serial.printf("Board: %s\n",
#if BOARD_S3
                "ESP32-S3 N16R8 (esp32-s3-devkitc-1)"
#else
                "Standard ESP32 / ESP32-D0WD-V3 DevKit (esp32dev)"
#endif
  );
  printPinAssignments();
  Serial.printf("MPU6050: %s\n",
                attitude.connected()
                    ? "detected"
                    : (attitude.demoMode() ? "not detected (test-mode demo active)"
                                            : "not detected"));
  Serial.printf("Wi-Fi SSID: %s\n", Config::kAccessPointName);
  Serial.printf("Wi-Fi password: %s\n", Config::kAccessPointPassword);
  Serial.printf("IP address: http://%s/\n",
                WiFi.softAPIP().toString().c_str());
  Serial.printf("Emergency stop: %s\n",
                emergencyStopLatched ? "LATCHED" : "clear");
  Serial.printf("Dead-man timeout: %lu ms\n",
                static_cast<unsigned long>(Config::kDeadmanTimeoutMs));
  Serial.printf("Default speed: %u%%  Max speed: %u%%\n",
                Config::kDefaultSpeedPercent, Config::kMaxSpeedPercent);
  Serial.println(currentMotion() == Motion::Stopped
                      ? "All motors confirmed STOPPED"
                      : "[fatal] motors did not start in STOPPED state");
}

void loop() {
  attitude.update();
  server.handleClient();
  delay(2);
}
