<div align="center">

   [![GitHub version](https://img.shields.io/github/release/Laxilef/OTGateway.svg?include_prereleases)](https://github.com/Laxilef/OTGateway/releases)
   [![GitHub download](https://img.shields.io/github/downloads/Laxilef/OTGateway/total.svg)](https://github.com/Laxilef/OTGateway/releases/latest)
   [![License](https://img.shields.io/github/license/Laxilef/OTGateway.svg)](LICENSE.txt)
   [![Telegram](https://img.shields.io/badge/Telegram-Channel-33A8E3)](https://t.me/otgateway)

</div>
<hr />

![Dashboard](/assets/poster-1.png) 
![Configuration](/assets/poster-2.png) 
![Integration with HomeAssistant](/assets/poster-3.png)

## Features
- DHW temperature control
- Heating temperature control
- Smart heating temperature control modes:
   - PID
   - Equithermic curves - adjusts the temperature based on indoor and outdoor temperatures
- Hysteresis setting _(for accurate maintenance of room temperature)_
- Ability to connect [additional (external) sensors](https://github.com/Laxilef/OTGateway/wiki/Compatibility#temperature-sensors): Dallas (1-wire), NTC 10k, Bluetooth (BLE). Makes it possible to monitor indoor and outdoor temperatures, temperatures on pipes/heat exchangers/etc.
- Emergency mode. In any dangerous situation _(loss of connection with Wifi, MQTT, sensors, etc)_ it will not let you and your home freeze.
- Ability of remote fault reset _(not with all boilers)_
- Diagnostics:
  - Displaying gateway status
  - Displaying the connection status to the boiler via OpenTherm
  - Displaying the fault status and fault code
  - Displaying the diagnostic status & diagnostic code
  - Display of the process of heating: works/does not work
  - Display of burner (flame) status: on/off
  - Display of burner modulation level in percent
  - Display of pressure in the heating system
  - Display of current temperature of the heat carrier
  - Display of return temperature of the heat carrier
  - Display of setpoint heat carrier temperature (useful when using PID or Equitherm)
  - Display of the process of DHW: working/not working
  - Display of current DHW temperature
  - _And other information..._
- [Home Assistant](https://www.home-assistant.io/) integration via MQTT. The ability to create any automation for the boiler!

## Documentation

### 当前部署（现场核实）

核实时间：**2026-11-14 08:38（Asia/Shanghai）**。以下为实机 API 和已登录的 Home Assistant 设备页读取结果；运行数值是当时快照。

| 项目 | 已确认信息 |
| --- | --- |
| 部署状态 | 实机已运行 OTGateway 固件，设备首页返回 HTTP 200 |
| 设备管理地址 | [http://192.168.100.171/](http://192.168.100.171/) |
| 主机名 | `opentherm` |
| 芯片 / 构建环境 | ESP32-C3（rev. 4）/ `c3_mini`，Flash 4 MiB |
| 实机固件版本 | `1.6.0-webfix1` |
| 固件构建时间 | `Nov 12 2026 12:41:49`（设备返回原文，时区未标注） |
| 壁挂炉连接 | OpenTherm 已连接：`/api/vars` 中 `slave.connected=true`，HA 中 `Opentherm status` 为“已连接” |
| MQTT 连接 | 已连接：`master.mqtt.connected=true` |
| Home Assistant 地址 | [https://home.winter-zhu.top:1009/](https://home.winter-zhu.top:1009/) |
| HA 设备 | `OpenTherm Gateway`，区域“厨房”，通过 MQTT 集成接入 |
| HA 设备详情 | [打开设备页](https://home.winter-zhu.top:1009/config/devices/device/3c0884e5293970374bfbac9ca126c8ac) |
| HA 与实机对应关系 | HA 设备“访问”链接指向 `http://192.168.100.171/`，固件版本同为 `1.6.0-webfix1` |

已确认固件在设备上运行；本次未比对实机固件与当前工作区源码/提交是否完全一致。壁挂炉品牌和型号尚未核实。

核实时的运行状态：

- 壁挂炉故障、诊断告警均未激活（`slave.fault.active=false`、`slave.diag.active=false`），紧急模式未激活（`master.emergency.state=false`）。
- 火焰、采暖、生活热水当时均未运行；HA 同样显示“未在运行”，OpenTherm 连接仍正常。
- 实机采暖水温约 43 °C，采暖水温设定值 45 °C，室温目标 23 °C、当前约 23.25 °C；生活热水温度 39 °C、目标 50 °C。
- HA 显示系统压力 1.30 bar，并已显示采暖、生活热水、火焰、温度和诊断等实体。

需要后续关注：`/api/info` 的上次重启原因是 `Software reset via esp_restart (critical heap)`，读取时已运行 677 秒，空闲堆内存 24,996 字节、历史最低 7,816 字节。该记录说明曾因堆内存临界触发软件重启；本次在线核实不能据此认定长期运行稳定，也未进一步定位重启根因。

复核入口（只读）：

```sh
curl --compressed --max-time 15 http://192.168.100.171/api/info
curl --compressed --max-time 15 http://192.168.100.171/api/vars
```

`/api/info` 用于查看固件、芯片、网络和重启信息；`/api/vars` 用于查看 OpenTherm、MQTT、故障和当前运行状态。HA 接入状态可登录上面的设备详情页核对。本次仅进行只读核实，没有调整设备、壁挂炉或 HA 设置。

### 异常重启修复记录

2026-11-14 的实机排查、修复、OTA 保护及测试结果见 [异常重启分析](docs/restart-analysis.md)。针对现场 ESP32-C3 的构建入口为 `python3 tools/build_stability.py`。

### 上游文档

All available information and instructions can be found in the wiki:

* [Home](https://github.com/Laxilef/OTGateway/wiki)
   * [Quick Start](https://github.com/Laxilef/OTGateway/wiki#quick-start)
   * [Build firmware](https://github.com/Laxilef/OTGateway/wiki#build-firmware)
   * [Flashing via Web Flasher](https://github.com/Laxilef/OTGateway/wiki#flashing-via-web-flasher)
   * [Flashing via ESP Flash Download Tool](https://github.com/Laxilef/OTGateway/wiki#flashing-via-esp-flash-download-tool)
   * [Settings](https://github.com/Laxilef/OTGateway/wiki#settings)
      * [External temperature sensors](https://github.com/Laxilef/OTGateway/wiki#external-temperature-sensors)
      * [Other external sensors](https://github.com/Laxilef/OTGateway/wiki#other-external-sensors)
      * [Reporting indoor/outdoor temperature from any Home Assistant sensor](https://github.com/Laxilef/OTGateway/wiki#reporting-indooroutdoor-temperature-from-any-home-assistant-sensor)
      * [Reporting outdoor temperature from Home Assistant weather integration](https://github.com/Laxilef/OTGateway/wiki#reporting-outdoor-temperature-from-home-assistant-weather-integration)
      * [DHW meter](https://github.com/Laxilef/OTGateway/wiki#dhw-meter)
      * [Advanced Settings](https://github.com/Laxilef/OTGateway/wiki#advanced-settings)
   * [Equitherm mode](https://github.com/Laxilef/OTGateway/wiki#equitherm-mode)
      * [Ratios](https://github.com/Laxilef/OTGateway/wiki#ratios)
      * [Fit coefficients](https://github.com/Laxilef/OTGateway/wiki#fit-coefficients)
   * [PID mode](https://github.com/Laxilef/OTGateway/wiki#pid-mode)
   * [Logs and debug](https://github.com/Laxilef/OTGateway/wiki#logs-and-debug)
* [Compatibility](https://github.com/Laxilef/OTGateway/wiki/Compatibility)
   * [Boilers](https://github.com/Laxilef/OTGateway/wiki/Compatibility#boilers)
   * [Boards](https://github.com/Laxilef/OTGateway/wiki/Compatibility#boards)
   * [Temperature sensors](https://github.com/Laxilef/OTGateway/wiki/Compatibility#temperature-sensors)
* [FAQ & Troubleshooting](https://github.com/Laxilef/OTGateway/wiki/FAQ-&-Troubleshooting)
* [OT adapters](https://github.com/Laxilef/OTGateway/wiki/OT-adapters)
   * [Adapters on sale](https://github.com/Laxilef/OTGateway/wiki/OT-adapters#adapters-on-sale)
   * [DIY](https://github.com/Laxilef/OTGateway/wiki/OT-adapters#diy)
      * [Files for production](https://github.com/Laxilef/OTGateway/wiki/OT-adapters#files-for-production)
      * [Connection](https://github.com/Laxilef/OTGateway/wiki/OT-adapters#connection)
      * [Leds on board](https://github.com/Laxilef/OTGateway/wiki/OT-adapters#leds-on-board)

## Gratitude
* To the developers of the libraries used: [OpenTherm Library](https://github.com/ihormelnyk/opentherm_library), [ESP8266Scheduler](https://github.com/nrwiersma/ESP8266Scheduler), [ArduinoJson](https://github.com/bblanchon/ArduinoJson), [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino), [ArduinoMqttClient](https://github.com/arduino-libraries/ArduinoMqttClient), [ESPTelnet](https://github.com/LennartHennigs/ESPTelnet), [FileData](https://github.com/GyverLibs/FileData), [GyverPID](https://github.com/GyverLibs/GyverPID), [GyverBlinker](https://github.com/GyverLibs/GyverBlinker), [OneWireNg](https://github.com/pstolarz/OneWireNg) & [OneWire](https://github.com/PaulStoffregen/OneWire)
* To the [PlatformIO](https://platformio.org/) Team
* To the team and contributors of the [pioarduino](https://github.com/pioarduino/platform-espressif32) project
* To the [BrowserStack](https://www.browserstack.com/) team. This project is tested with BrowserStack.
* To the [PVS-Studio](https://pvs-studio.com/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source) - static analyzer for C, C++, C#, and Java code.
* And of course to the contributors for their contribution to the development of the project!
