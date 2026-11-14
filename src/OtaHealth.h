#pragma once

#ifdef ARDUINO_ARCH_ESP32
#include <esp_ota_ops.h>

// Arduino otherwise validates the image before setup() has connected Wi-Fi.
extern "C" bool verifyRollbackLater() {
  return true;
}
#endif

namespace OtaHealth {
#ifdef ARDUINO_ARCH_ESP32
  static bool pending = false;
  static unsigned long started = 0;
  static unsigned long healthySince = 0;
  static esp_err_t lastError = ESP_OK;
  static bool rollbackAttempted = false;
  static esp_err_t recoveryError = ESP_OK;

  void begin() {
    started = millis();
    healthySince = 0;
    rollbackAttempted = false;
    esp_ota_img_states_t state;
    const auto* running = esp_ota_get_running_partition();
    pending = running && esp_ota_get_state_partition(running, &state) == ESP_OK
      && state == ESP_OTA_IMG_PENDING_VERIFY;
  }

  void loop(bool healthy) {
    if (!pending) {
      return;
    }

    const unsigned long now = millis();
    if (!healthy) {
      healthySince = 0;
    } else if (!healthySince) {
      healthySince = now;
    }

    // Include BLE startup and MQTT discovery in the probation period.
    if (healthySince && now - healthySince >= 120000u) {
      lastError = esp_ota_mark_app_valid_cancel_rollback();
      if (lastError == ESP_OK) {
        pending = false;
        return;
      }
    }

    if (!rollbackAttempted && now - started >= 300000u) {
      rollbackAttempted = true;
      lastError = esp_ota_mark_app_invalid_rollback_and_reboot();
      // No usable fallback: keep this image bootable and unlock OTA recovery.
      // Otherwise pending-verification would permanently reject new uploads.
      if (lastError != ESP_OK) {
        recoveryError = esp_ota_mark_app_valid_cancel_rollback();
        if (recoveryError == ESP_OK) {
          pending = false;
        }
      }
    }
  }

  bool isPending() { return pending; }

  void toJson(JsonObject dst) {
    const auto* running = esp_ota_get_running_partition();
    const auto* next = esp_ota_get_next_update_partition(nullptr);
    esp_ota_img_states_t state;
    dst["runningPartition"] = running ? running->label : "";
    dst["nextPartition"] = next ? next->label : "";
    dst["slotSize"] = next ? next->size : 0;
    dst["pendingVerification"] = pending;
    dst["lastError"] = lastError;
    dst["recoveryError"] = recoveryError;
    if (running && esp_ota_get_state_partition(running, &state) == ESP_OK) {
      dst["imageState"] = static_cast<int>(state);
    }
  }
#else
  void begin() {}
  void loop(bool) {}
  bool isPending() { return false; }
#endif
}
