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

  lastValidMotionMs = millis();
  applyJoystick(xPercent, yPercent);
  motionActive = currentMotion() != Motion::Stopped;
  sendJson(200, statusJson());
}

void handleStop() {
  motionActive = false;
  stopForSafety("stop command");
  sendJson(200, statusJson());
}

void handleEmergencyStop() {
  emergencyStopLatched = true;
  motionActive = false;
  stopForSafety("emergency stop");
  Serial.println("[safety] emergency stop latched");
  sendJson(200, statusJson());
}

void handleClearEmergencyStop() {
  // Clearing the latch never starts motion; a new hold-to-run command is needed.
  motionActive = false;
  stopForSafety("clear emergency stop");
  emergencyStopLatched = false;
  Serial.println("[safety] emergency stop cleared; motors remain stopped");
  sendJson(200, statusJson());
}

void handleSetLevel() {
  if (!attitude.setLevel()) {
    sendJson(503, F("{\"error\":\"MMA8452Q is not connected\"}"));
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
  server.on("/api/attitude/level", HTTP_POST, handleSetLevel);
  server.onNotFound([]() {
    stopForSafety("unknown request");
    sendJson(404, F("{\"error\":\"Not found\"}"));
  });
  server.begin();
}

}  // namespace

void setup() {
  // Motor safety is established before Wi-Fi or the web server starts.
  Serial.begin(115200);
  motors.begin();
  attitude.begin();
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
  Serial.printf("Mode: %s\n", TEST_MODE ? "TEST (no motor hardware required)"
                                       : "HARDWARE");
  Serial.printf("Wi-Fi: %s\n", Config::kAccessPointName);
  Serial.printf("Open: http://%s/\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("Dead-man timeout: %lu ms\n",
                static_cast<unsigned long>(Config::kDeadmanTimeoutMs));
}

void loop() {
  attitude.update();
  server.handleClient();
  delay(2);
}
