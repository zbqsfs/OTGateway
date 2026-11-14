#!/usr/bin/env python3
"""Build only the C3 OTA application using the deployed Arduino 3.3.5 baseline."""
import configparser
from pathlib import Path
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
output = root / 'build' / 'restart-investigation'
output.mkdir(parents=True, exist_ok=True)
config = configparser.ConfigParser(interpolation=None, inline_comment_prefixes=(';',))
config.read(root / 'platformio.ini')
config['env']['version'] = '1.6.0-stability3'
config['esp32_defaults']['platform'] = 'https://github.com/pioarduino/platform-espressif32/releases/download/55.03.35/platform-espressif32.zip'
# Do not build a factory image or filesystem as part of this OTA repair.
config['esp32_defaults']['extra_scripts'] = ''
# Pin the dependency revisions used for this hardware validation.
revisions = {
    'ArduinoJson@^7.4.2': 'ArduinoJson@7.4.3',
    'ArduinoMqttClient@^0.1.8': 'ArduinoMqttClient@0.1.8',
    'ESP Telnet@^2.2.3': 'ESP Telnet@2.3.0',
    'FileData@^1.0.3': 'FileData@1.0.4',
    'GyverPID@^3.3.2': 'GyverPID@3.3.2',
    'GyverBlinker@^1.1.1': 'GyverBlinker@1.1.2',
    'OneWireNg@^0.14.1': 'OneWireNg@0.14.1',
    'DallasTemperature@^4.0.6': 'DallasTemperature@4.0.6',
    'TinyLogger@^1.1.1': 'TinyLogger@1.1.1',
    '#esp32_timer': '#4a27e179c4fd14fed5c789bd069d87617d4c3959',
}
for old, new in revisions.items():
    config['env']['lib_deps'] = config['env']['lib_deps'].replace(old, new)
config['esp32_defaults']['lib_deps'] = config['esp32_defaults']['lib_deps'].replace('ESP32Scheduler@^1.0.1', 'ESP32Scheduler@1.0.1')
config_path = output / 'build.ini'
with config_path.open('w') as stream:
    config.write(stream)
pio = root / '.pio' / 'penv' / 'bin' / 'pio'
subprocess.run([str(pio) if pio.exists() else 'pio', 'run', '-c', str(config_path), '-e', 'c3_mini'], cwd=root, check=True)
firmware = root / '.pio' / 'build' / 'c3_mini' / 'firmware.bin'
if firmware.stat().st_size > 0x1B0000:
    raise RuntimeError('Firmware exceeds the deployed OTA slot')
for suffix in ('bin', 'elf'):
    shutil.copy2(firmware.with_suffix('.' + suffix), output / ('firmware_c3_mini_stability3.' + suffix))
print('OTA application: ' + str(output / 'firmware_c3_mini_stability3.bin'))
