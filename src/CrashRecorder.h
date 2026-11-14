#pragma once
#include <Arduino.h>

#ifdef ARDUINO_ARCH_ESP32
  #include "esp_attr.h"
  #include "esp_err.h"
  #include "esp_system.h"
#endif

#ifdef ARDUINO_ARCH_ESP8266
  extern "C" {
    #include <user_interface.h>
  }

  // https://github.com/espressif/ESP8266_RTOS_SDK/blob/master/components/esp8266/include/esp_attr.h
  #define _COUNTER_STRINGIFY(COUNTER) #COUNTER
  #define _SECTION_ATTR_IMPL(SECTION, COUNTER) __attribute__((section(SECTION "." _COUNTER_STRINGIFY(COUNTER))))
  #define __NOINIT_ATTR _SECTION_ATTR_IMPL(".noinit", __COUNTER__)
#endif

namespace CrashRecorder {
  typedef struct {
    unsigned int data[32];
    uint8_t length;
    bool continues;
  } backtrace_t;

  typedef struct {
    unsigned int data[4];
    uint8_t length;
  } epc_t;

  typedef struct {
    uint8_t core;
    size_t heap;
    unsigned long uptime;
  } ext_t;

  typedef struct {
    uint32_t magic;
    size_t heap;
    size_t maxFreeBlock;
    unsigned long uptime;
  } heap_restart_t;

  const uint32_t heapRestartMagic = 0x48454150;
  #ifdef ARDUINO_ARCH_ESP32
  RTC_NOINIT_ATTR volatile static heap_restart_t pendingHeapRestart;
  #else
  __NOINIT_ATTR volatile static heap_restart_t pendingHeapRestart;
  #endif
  static heap_restart_t heapRestart = {};
  bool heapRestartAvailable = false;

  void recordHeapRestart(size_t heap, size_t maxFreeBlock, unsigned long uptime) {
    pendingHeapRestart.magic = 0;
    pendingHeapRestart.heap = heap;
    pendingHeapRestart.maxFreeBlock = maxFreeBlock;
    pendingHeapRestart.uptime = uptime;
    pendingHeapRestart.magic = heapRestartMagic;
  }


  __NOINIT_ATTR volatile static backtrace_t backtrace;
  __NOINIT_ATTR volatile static epc_t epc;
  __NOINIT_ATTR volatile static ext_t ext;

  uint8_t backtraceMaxLength = sizeof(backtrace.data) / sizeof(*backtrace.data);
  uint8_t epcMaxLength = sizeof(epc.data) / sizeof(*epc.data);

  #ifdef ARDUINO_ARCH_ESP32
  void IRAM_ATTR panicHandler(arduino_panic_info_t *info, void *arg) {;
    ext.core = info->core;
    ext.heap = ESP.getFreeHeap();
    ext.uptime = millis() / 1000u;

    // Backtrace
    backtrace.length = info->backtrace_len < backtraceMaxLength ? info->backtrace_len : backtraceMaxLength;
    backtrace.continues = false;
    for (unsigned int i = 0; i < info->backtrace_len; i++) {
      if (i >= backtraceMaxLength) {
        backtrace.continues = true;
        break;
      }

      backtrace.data[i] = info->backtrace[i];
    }

    // EPC
    if (info->pc) {
      epc.data[0] = (unsigned int) info->pc;
      epc.length = 1;

    } else {
      epc.length = 0;
    }
  }
  #endif
  
  void init() {
    #ifdef ARDUINO_ARCH_ESP32
    const bool softwareRestart = esp_reset_reason() == ESP_RST_SW;
    #else
    const bool softwareRestart = ESP.getResetInfoPtr()->reason == REASON_SOFT_RESTART;
    #endif

    heapRestartAvailable = softwareRestart && pendingHeapRestart.magic == heapRestartMagic;
    if (heapRestartAvailable) {
      heapRestart.heap = pendingHeapRestart.heap;
      heapRestart.maxFreeBlock = pendingHeapRestart.maxFreeBlock;
      heapRestart.uptime = pendingHeapRestart.uptime;
    }
    // Consume the marker so a later manual restart cannot inherit this cause.
    pendingHeapRestart.magic = 0;

    if (backtrace.length > backtraceMaxLength) {
      backtrace.length = 0;
    }

    if (epc.length > epcMaxLength) {
      epc.length = 0;
    }

    #ifdef ARDUINO_ARCH_ESP32
    set_arduino_panic_handler(panicHandler, nullptr);
    #endif
  }
}

#ifdef ARDUINO_ARCH_ESP8266
extern "C" void custom_crash_callback(struct rst_info *info, uint32_t stack, uint32_t stack_end) {
  uint8_t _length = 0;

  CrashRecorder::ext.core = 0;
  CrashRecorder::ext.heap = ESP.getFreeHeap();
  CrashRecorder::ext.uptime = millis() / 1000u;

  // Backtrace
  CrashRecorder::backtrace.continues = false;
  uint32_t value;
  for (uint32_t i = stack; i < stack_end; i += 4) {
    value = *((uint32_t*) i);

    // keep only addresses in code area
    if ((value >= 0x40000000) && (value < 0x40300000)) {
      if (_length >= CrashRecorder::backtraceMaxLength) {
        CrashRecorder::backtrace.continues = true;
        break;
      }

      CrashRecorder::backtrace.data[_length++] = value;
    }
  }

  CrashRecorder::backtrace.length = _length;

  // EPC
  _length = 0;
  if (info->epc1 > 0) {
    CrashRecorder::epc.data[_length++] = info->epc1;
  }

  if (info->epc2 > 0) {
    CrashRecorder::epc.data[_length++] = info->epc2;
  }

  if (info->epc3 > 0) {
    CrashRecorder::epc.data[_length++] = info->epc3;
  }

  CrashRecorder::epc.length = _length;
}
#endif
