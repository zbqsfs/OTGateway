#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
out=build/restart-investigation
mkdir -p "$out"
clang++ -std=c++17 -fsanitize=address,undefined -g \
  -DARDUINO_ARCH_ESP32 -DARDUINOJSON_POOL_CAPACITY=64 \
  -I . -I tests/host/stubs -I .pio/libdeps/c3_mini/ArduinoJson/src \
  tests/host/stability.cpp -o "$out/host-tests"
"$out/host-tests"
clang++ -std=c++17 -fsanitize=address,undefined -g \
  -I . -I tests/host/stubs -I .pio/libdeps/c3_mini/FileData/src \
  tests/host/network_storage.cpp -o "$out/network-storage-tests"
"$out/network-storage-tests"
clang++ -std=c++17 -fsanitize=address,undefined -g \
  -I . -I tests/host/stubs -I .pio/libdeps/c3_mini/FileData/src \
  tests/host/settings_migration.cpp -o "$out/settings-migration-tests"
"$out/settings-migration-tests"
clang++ -std=c++17 -fsanitize=address,undefined -g -DARDUINO_ARCH_ESP32 \
  -I . -I tests/host/stubs tests/host/upgrade.cpp -o "$out/upgrade-tests"
"$out/upgrade-tests"
