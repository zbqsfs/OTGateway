# 设备异常重启排查（2026-11-14）

最终固件：**1.6.0-stability3**。源码修复和 Wi-Fi OTA 已完成，最终连续观察超过 16 分钟、无异常重启；页面/API 负载测试和正常运行状态下再次 OTA 均通过。固件文件：`build/restart-investigation/firmware_c3_mini_stability3.bin`；仅用于本次 ESP32-C3 环境的应用 OTA。

## 已确认事实

现场设备：`192.168.100.171`，ESP32-C3 rev.4，4 MiB Flash，原固件 `1.6.0-webfix1`，Arduino 3.3.5 / ESP-IDF v5.5.1。现场使用 Wi-Fi、MQTT/HA 和一个 BLE 室温传感器。

排查期间 `/api/debug` 捕获到多次 `critical_heap`：

| 触发时运行秒数 | 空闲堆（字节） | 最大连续空闲块（字节） |
|---:|---:|---:|
| 53 | 4,124 | 1,268 |
| 959 | 4,108 | 1,140 |
| 298 | 2,036 | 1,524 |

原固件平时空闲堆约 24–28 KB，启动前期约 94 KB；历史最低曾读到 304、268 字节。`MainTask::heap()` 在空闲堆或最大连续块小于 2,048 字节时设置重启标记，15 秒后 `ESP.restart()`。这是这些已捕获重启的直接原因；没有证据把它们归因于供电掉电或看门狗。

原来的 `minMaxFreeBlock` 混入了总空闲堆的历史最低值，不能把该指标当作真实的最大连续块历史最小值。修复后该字段仅记录采样得到的最大连续块最小值。

## 内存压力来源与修复

1. **网页发送碎片过多**：`PortalTask` 用 32 字节发送缓冲，且启用了 TCP_NODELAY。一个数 KB 的 JSON 会产生上百次小写入，增加 TCP/Wi-Fi 缓冲开销，并可能与 MQTT/BLE 分配同时发生。改成 1,024 字节合并发送。保留网络协议和现有页面。
2. **JSON 临时分配**：ESP32 上 ArduinoJson 的默认池为 128 个槽；改为 64 个槽，让小文档按较小粒度分配，大文档仍可按需增长。发送前检查 `overflowed()`，分配失败返回 HTTP 503，避免把不完整数据当作成功响应。
3. **固定任务栈**：ESP32Scheduler 1.0.1 默认为每任务预留 10,000 字节，六个任务共 60,000 字节。新增 `/api/debug` 的 `stackMinFreeBytes`，用实际高水位决定是否调整，避免盲目缩栈。
4. **已存在的网页越界修复**：保留工作区原有 MD5 ETag 缓冲修正（32 个十六进制字符加 NUL 需要 33 字节）和低内存重启记录。
5. **升级错误日志**：修正 `%s` 接收 `String` 对象的错误，改传 `c_str()`。

这说明存在明确的内存容量与瞬时峰值问题；目前没有通过堆追踪证实持续泄漏，不能把“内存不足”直接等同于“内存泄漏”。

## 保住 Wi-Fi OTA 的措施

- 只上传应用 `.bin`，不上传 `.factory.bin`、文件系统、分区表或引导程序。OTA 槽大小 `0x1B0000`（1,769,472 字节）。
- 沿用现场 Arduino 3.3.5 / ESP-IDF v5.5.1，避免在稳定性修复中同时升级框架。
- 新增 `verifyRollbackLater` 的 **C 链接强符号**，防止 Arduino 在 `setup()` 前直接确认新镜像。网络已连接、Portal 已启动且内存有余量，连续健康 120 秒后才确认；启动 300 秒内未通过则调用回退 API。若没有可用回退镜像，保留当前运行以便通过网页恢复，并在 debug 暴露错误。
- 仅在设备报告 `ESP_OTA_IMG_PENDING_VERIFY` 时执行上述确认流程；不能单凭本地配置断言现场引导程序具备自动回退。[Espressif OTA 状态与回退文档](https://docs.espressif.com/projects/esp-idf/en/release-v5.5/esp32c3/api-reference/system/ota.html)说明了该前提。
- 待验证阶段禁止再次 OTA，以免覆盖备用固件。升级期间不新触发低内存主动重启。
- **兼容旧版网络配置格式**：当前源码增加了 Ethernet 字段；FileData 默认会扩展旧文件，而旧固件回退后会重置过大的配置文件，导致 Wi-Fi 凭据丢失。Wi-Fi-only 构建现在保留已有旧格式长度，读、改、存后旧版仍能读回；已有新版扩展格式也不缩短。
- 已将现场配置备份到本机忽略目录 `build/restart-investigation/backup.json`（含凭据，权限 0600，所在目录权限 0700），不提交版本库。

首次上传被原固件的重启保护拒绝（HTTP 400，firmware status 3），未成功激活新镜像。后续使用受控重启后的早期窗口，在 BLE 初始化前上传。

## 本地验证

- `tests/host/run.sh`：AddressSanitizer + UndefinedBehaviorSanitizer。验证大 JSON 内容及分包数、内存分配失败、OTA 延迟确认、健康计时重置、回退失败处理、旧网络文件升级/回退后凭据保持、新格式保留。
- `c3_mini` 实机目标编译通过。
- `d1_mini`（ESP8266）兼容性编译通过；未对 ESP8266 实机刷写。
- 镜像经 esptool 检查芯片 ESP32-C3、DIO/4 MB、校验和和验证哈希；检查 ELF 中 `verifyRollbackLater` 为强符号 `T`。

## 复现与后续观察

```sh
python3 tools/build_stability.py
sh tests/host/run.sh
python3 tools/monitor_stability.py http://192.168.100.171 --seconds 3600 --interval 10
```

构建工具只生成应用固件；监控工具只读取 API，并输出 uptime、版本、重启原因和堆指标，不修改锅炉配置。依赖首次构建需要网络；构建日志保存在本次本地验证目录。长期运行还应覆盖 BLE 断连重连、Wi-Fi/MQTT 断连重连、正常采暖/生活热水周期；短期在线测试不能代替多天运行验证。

## 现场验证中追加修复

首轮 `stability1` 成功启动且报告 `pendingVerification=true` / `imageState=1`，因此确认现场引导程序支持 OTA 待验证/回退。旧网络文件保持 289 字节（含 1 字节标记），未丢失 Wi-Fi。

同时发现当前源码自 `cbc32fd` 起修改了锅炉设置结构：旧 `Settings` 为 556 字节，新版为 552 字节。原 FileData 会因长度不符重置设置。首轮已通过备份恢复锅炉 GPIO、温度、PID、MQTT 等设置，并恢复原先禁用的两个 LED。新版本加入经过验证的旧格式迁移，写入 `/settings-v2.conf`，保留 `/settings.conf` 给旧固件回退；迁移前验证原数据标记和尾部 `stvalid`。

注意仓库已有的防冻策略变更：旧字段 `thresholdTime=600` 被上游删除，新策略使用温度迟滞；迁移保留现场 `lowTemp=5 °C`，新 `highTemp=15 °C` 使用当前源码默认值。旧文件仍保留旧时延设置。该差异属于当前源码与现场旧版本的差异，并非内存修复必须改变的参数，需在后续控制策略审查中留意。

首轮在恢复业务后内存仍不足以连续通过健康门槛（历史最低 700 字节），因此在约 300 秒自动回退到 `webfix1`；回退后 Wi-Fi 可访问，已恢复原设置并受控重启。这验证了回退通路，但也表明仅减小 JSON/网络发送峰值不足，必须增加常驻可用内存。

六个 10,000 字节任务栈实测历史剩余：Main 6,840、OpenTherm 7,852、Sensors 7,512、Regulator 7,480、Portal 7,308、MQTT 7,848 字节。第二轮仅对 ESP32-C3 调整为 Main/OpenTherm/Regulator 各 6,144，Sensors/Portal/MQTT 各 8,192，合计释放 16,992 字节；其他 ESP32 目标保留原默认值。

第二轮 `stability2` 连续约七分钟运行无异常重启，通过启动健康确认（`imageState=2`）。两路并发的 100 个页面/API 请求和 50 次健康检查全部 HTTP 200，但历史堆最低仍到过 1,136 字节。因此最终版进一步保守缩栈至 Main/MQTT 6,144/8,192，OpenTherm/Regulator 各 4,096，Sensors/Portal 各 6,144，合计 34,816 字节，比原版释放 **25,184 字节**。实测 Main 使用约 3,168 字节；其他任务在第二轮已测路径使用约 876–2,720 字节。`BufferedWebServer` 每次批量发送后让出 1 ms，给 TCP/Wi-Fi 释放缓冲的机会。

最终版还处理两个恢复边界：上传连接中断时清理 Update 状态并解除 `upgrading`，允许业务恢复和重试；如果 OTA 回退失败且没有可用旧镜像，尝试确认当前镜像以保留再次 OTA 的机会，并公开 `recoveryError`，不无限封锁上传。上传中断、出错后中断与重试均有主机侧回归测试；没有在现场故意制造断电或损坏镜像。

最终版正常运行状态下 OTA 成功（无需抢启动窗口），上传 1,660,128 字节应用耗时约 130 秒；早期启动窗口上传约 10–11 秒。升级后从 app0 切换到 app1，健康检查通过，`imageState=2`、`pendingVerification=false`、`lastError=0`、`recoveryError=0`。

最终版两路并发测试：100 个页面/API 请求 + 50 次健康检查，全数成功，耗时 32.82 秒，历史最低堆为 12,284 字节、采样最大连续块最低 9,204 字节，无异常重启。可复测：

```sh
python3 tests/hardware/http_load.py http://192.168.100.171 --version 1.6.0-stability3
```

最终固件 SHA-256：`9e50fcd640ecffa8e2320386d74a9156931e1b0e9a6cb1fb131cf8ef824cdb27`。镜像为 1,660,128 字节，低于 1,769,472 字节 OTA 槽。源码文件哈希和构建/实机原始记录保存在本机忽略目录 `build/restart-investigation/`。

## 最终验收快照

核实时间：2026-11-14T09:30:01.178859+08:00（Asia/Shanghai）。最终固件已运行 1027 秒，连续采样覆盖 994 秒，99 次读数，无异常重启、无读取失败。当前空闲堆 45824 字节，历史最低 12,284 字节，最大连续块 36852 字节。

最终任务栈历史剩余（字节）：Main 2988, OpenTherm 3220, Sensors 3800, Regulator 3572, Portal 3536, Mqtt 6036。

OpenTherm、MQTT、BLE 均已连接；最终 OTA 前后完整配置完全一致。相对最初旧版备份，除上文已说明的上游防冻策略字段变化外，既有网络、锅炉与传感器参数一致。本次没有刷写文件系统、分区表或引导程序，没有使用 USB。长期、断网重连和真实供暖/生活热水负载周期仍需继续观察；本次结果不代表已排除所有未来重启原因。
