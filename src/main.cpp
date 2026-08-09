#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#include "config.h"
#include "motor_controller.h"
#include "web_page.h"

namespace {

WebServer server(Config::kHttpPort);
MotorController motors;
SemaphoreHandle_t motorMutex = nullptr;
bool emergencyStopLatched = false;
volatile bool motionActive = false;
volatile uint32_t lastValidMotionMs = 0;

Motion currentMotion() {
  if (motorMutex == nullptr ||
      xSemaphoreTake(motorMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
    return Motion::Stopped;
  }
  const Motion result = motors.motion();
  xSemaphoreGive(motorMutex);
  return result;
}

void applyMotion(Motion motion) {
  if (motorMutex == nullptr ||
      xSemaphoreTake(motorMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
    motionActive = false;
    return;
  }
  motors.apply(motion);
  xSemaphoreGive(motorMutex);
}

String statusJson() {
  String json;
  json.reserve(100);
  json = F("{\"motion\":\"");
  json += motionName(currentMotion());
  json += F("\",\"estop\":");
  json += emergencyStopLatched ? F("true") : F("false");
  json += F(",\"testMode\":");
#if TEST_MODE
  json += F("true");
#else
  json += F("false");
#endif
  json += F("}");
  return json;
}

void sendJson(int code, const String& body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", body);
}

void stopForSafety(const char* reason) {
  if (motorMutex == nullptr ||
      xSemaphoreTake(motorMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
    return;
  }
  const bool wasMoving = motors.motion() != Motion::Stopped;
  motors.stop();
  xSemaphoreGive(motorMutex);
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

void configureServer() {
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html", kControlPage);
  });
  server.on("/api/status", HTTP_GET,
            []() { sendJson(200, statusJson()); });
  server.on("/api/move", HTTP_POST, handleMove);
  server.on("/api/stop", HTTP_POST, handleStop);
  server.on("/api/estop", HTTP_POST, handleEmergencyStop);
  server.on("/api/estop/clear", HTTP_POST, handleClearEmergencyStop);
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
  server.handleClient();
  delay(2);
}
