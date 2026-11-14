class BufferedWebServer {
public:
  BufferedWebServer(WebServer* webServer, size_t bufferSize = 64) {
    this->webServer = webServer;
    this->bufferSize = bufferSize;
    this->buffer = (uint8_t*)malloc(bufferSize * sizeof(*this->buffer));
  }

  ~BufferedWebServer() {
    free(this->buffer);
  }

  template <class T>
  void send(int code, T contentType, const JsonDocument& content, bool pretty = false) {
    if (content.overflowed() || this->buffer == nullptr || this->bufferSize == 0) {
      this->webServer->send(503, "text/plain", "Insufficient memory; retry later");
      return;
    }
    auto contentLength = pretty ? measureJsonPretty(content) : measureJson(content);

    #ifdef ARDUINO_ARCH_ESP8266
    if (!this->webServer->chunkedResponseModeStart(code, contentType)) {
      this->webServer->send(505, F("text/html"), F("HTTP1.1 required"));
      return;
    }

    this->webServer->setContentLength(contentLength);
    #else
    this->webServer->setContentLength(contentLength);
    this->webServer->send(code, contentType, emptyString);
    #endif

    if (pretty) {
      serializeJsonPretty(content, *this);
      
    } else {
      serializeJson(content, *this);
    }

    this->flush();

    #ifdef ARDUINO_ARCH_ESP8266
    this->webServer->chunkedResponseFinalize();
    #else
    this->webServer->sendContent(emptyString);
    #endif
  }

  size_t write(uint8_t c) {
    this->buffer[this->bufferPos++] = c;

    if (this->bufferPos >= this->bufferSize) {
      this->flush();
    }

    return 1;
  }

  size_t write(const uint8_t* buffer, size_t length) {
    size_t written = 0;
    while (written < length) {
      size_t copySize = this->bufferSize - this->bufferPos;
      if (written + copySize > length) {
        copySize = length - written;
      }

      memcpy(this->buffer + this->bufferPos, buffer + written, copySize);
      this->bufferPos += copySize;

      if (this->bufferPos >= this->bufferSize) {
        this->flush();
      }

      written += copySize;
    }

    return written;
  }

  void flush() {
    if (this->bufferPos == 0) {
      return;
    }

    #ifdef ARDUINO_ARCH_ESP8266
    ::optimistic_yield(1000);
    #endif

    auto& client = this->webServer->client();
    if (client.connected()) {
      this->webServer->sendContent((const char*)this->buffer, this->bufferPos);
    }

    this->bufferPos = 0;

    #ifdef ARDUINO_ARCH_ESP8266
    ::optimistic_yield(1000);
    #elif defined(ARDUINO_ARCH_ESP32)
    // Let TCP/Wi-Fi drain between batches instead of retaining the whole
    // response in network buffers while MQTT and BLE are also allocating.
    ::delay(1);
    #endif
  }

protected:
  WebServer* webServer = nullptr;
  uint8_t* buffer;
  size_t bufferSize = 64;
  size_t bufferPos = 0;
};
