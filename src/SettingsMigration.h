#pragma once
#include <stddef.h>
#include <string.h>

// Before cbc32fd, freezeProtection was { uint8_t lowTemp; uint16_t
// thresholdTime; }. The new layout is four bytes shorter after alignment.
// Keep the original file for the previous OTA image; migrate into a new file.
template <typename T>
FDstat_t readSettingsPreservingRollback(FileData& storage, fs::FS& fs, T& settings) {
  constexpr size_t legacySize = 556;
  constexpr size_t freezeOffset = offsetof(T, heating.freezeProtection);
  constexpr size_t suffixOffset = offsetof(T, dhw);
  static_assert(sizeof(T) == 552 && freezeOffset == 426 && suffixOffset == 428,
    "Settings layout changed: review legacy migration before deploying");
  const char* migratedPath = "/settings-v2.conf";
  if (fs.exists(migratedPath)) {
    storage.setFS(&fs, migratedPath);
    return storage.read();
  }

  File file = fs.open("/settings.conf", "r");
  if (file && file.size() == legacySize + 1 && file.read() == 's') {
    uint8_t legacy[legacySize];
    if (file.read(legacy, sizeof(legacy)) != sizeof(legacy)
        || memcmp(legacy + 548, SETTINGS_VALID_VALUE, 8) != 0) {
      return FD_FILE_ERR;
    }
    file.close();
    memcpy(&settings, legacy, freezeOffset);
    settings.heating.freezeProtection.lowTemp = legacy[freezeOffset];
    if (settings.heating.freezeProtection.highTemp < settings.heating.freezeProtection.lowTemp) {
      settings.heating.freezeProtection.highTemp = settings.heating.freezeProtection.lowTemp;
    }
    memcpy(reinterpret_cast<uint8_t*>(&settings) + suffixOffset,
      legacy + suffixOffset + 4, sizeof(T) - suffixOffset);
    storage.setFS(&fs, migratedPath);
    const auto result = storage.write();
    return result == FD_WRITE ? FD_READ : result;
  }
  file.close();
  return storage.read();
}
