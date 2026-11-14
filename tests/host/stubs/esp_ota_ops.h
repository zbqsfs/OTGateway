#pragma once
#include <cstdint>
using esp_err_t = int;
constexpr esp_err_t ESP_OK = 0;
enum esp_ota_img_states_t { ESP_OTA_IMG_NEW, ESP_OTA_IMG_PENDING_VERIFY, ESP_OTA_IMG_VALID };
struct esp_partition_t { const char* label; uint32_t size; };
extern unsigned long fakeMillis;
extern int confirmations, rollbacks;
extern esp_err_t confirmError, rollbackError;
extern esp_ota_img_states_t imageState;
inline unsigned long millis() { return fakeMillis; }
inline const esp_partition_t* esp_ota_get_running_partition() { static esp_partition_t p{"app0", 0x1B0000}; return &p; }
inline const esp_partition_t* esp_ota_get_next_update_partition(void*) { static esp_partition_t p{"app1", 0x1B0000}; return &p; }
inline esp_err_t esp_ota_get_state_partition(const esp_partition_t*, esp_ota_img_states_t* out) { *out=imageState; return ESP_OK; }
inline esp_err_t esp_ota_mark_app_valid_cancel_rollback() { ++confirmations; return confirmError; }
inline esp_err_t esp_ota_mark_app_invalid_rollback_and_reboot() { ++rollbacks; return rollbackError; }
