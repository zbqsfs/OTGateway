#pragma once
#include <stddef.h>

// FileData expands smaller records on read and resets larger ones to defaults.
// Wi-Fi-only builds must retain the legacy prefix to remain OTA-rollback-safe.
template <typename T>
void preserveWifiNetworkLayout(FileData& storage, fs::FS& fs, T& settings) {
  File file = fs.open("/network.conf", "r");
  const size_t wifiSettingsSize = offsetof(T, ethernet);
  if (file && file.size() == wifiSettingsSize + 1 && file.read() == 'n') {
    storage.setData(&settings, wifiSettingsSize);
  }
}
