#include <ArduinoJson.h>
#include <cassert>
#include <cstring>
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
constexpr const char* emptyString = "";
void delay(unsigned long) {}
struct Client { bool connected() const { return true; } };
struct WebServer {
  Client connection;
  int code = 0;
  size_t contentLength = 0;
  std::string body;
  std::vector<size_t> writes;
  Client& client() { return connection; }
  void setContentLength(size_t n) { contentLength=n; }
  void send(int status, const char*, const char* data) { code=status; body=data; writes.clear(); }
  void sendContent(const char* data, size_t n) { body.append(data,n); if(n) writes.push_back(n); }
  void sendContent(const char* data) { sendContent(data,strlen(data)); }
};
#include "lib/BufferedWebServer/BufferedWebServer.h"
#include "src/OtaHealth.h"
unsigned long fakeMillis=1;
int confirmations=0, rollbacks=0;
esp_err_t confirmError=ESP_OK, rollbackError=ESP_OK;
esp_ota_img_states_t imageState=ESP_OTA_IMG_PENDING_VERIFY;
struct FailingAllocator : ArduinoJson::Allocator {
  void* allocate(size_t) override { return nullptr; }
  void deallocate(void*) override {}
  void* reallocate(void*, size_t) override { return nullptr; }
};
void resetOta() {
  fakeMillis=1; confirmations=rollbacks=0; confirmError=rollbackError=ESP_OK;
  imageState=ESP_OTA_IMG_PENDING_VERIFY; OtaHealth::healthySince=0; OtaHealth::begin();
}
int main() {
  WebServer server;
  BufferedWebServer buffered(&server,1024);
  JsonDocument doc;
  doc["payload"]=std::string(6000,'x');
  for (bool pretty : {false,true}) {
    std::string expected;
    if(pretty) serializeJsonPretty(doc,expected); else serializeJson(doc,expected);
    buffered.send(200,"application/json",doc,pretty);
    assert(server.code==200 && server.body==expected && server.contentLength==expected.size());
    assert(server.writes.size()==(expected.size()+1023)/1024);
    assert(*std::max_element(server.writes.begin(),server.writes.end())<=1024);
  }
  FailingAllocator allocator;
  JsonDocument failed(&allocator); failed["value"]=1;
  assert(failed.overflowed());
  buffered.send(200,"application/json",failed);
  assert(server.code==503);
  BufferedWebServer zero(&server,0);
  zero.send(200,"application/json",doc);
  assert(server.code==503);
  resetOta(); assert(verifyRollbackLater());
  OtaHealth::loop(true); fakeMillis=120000; OtaHealth::loop(true); assert(confirmations==0);
  fakeMillis=120001; OtaHealth::loop(true); assert(confirmations==1 && !OtaHealth::pending);
  resetOta(); OtaHealth::loop(true); fakeMillis=110000; OtaHealth::loop(false);
  fakeMillis=120000; OtaHealth::loop(true); fakeMillis=239999; OtaHealth::loop(true); assert(confirmations==0);
  fakeMillis=240000; OtaHealth::loop(true); assert(confirmations==1);
  resetOta(); fakeMillis=300001; rollbackError=-1; OtaHealth::loop(false);
  assert(rollbacks==1 && !OtaHealth::pending && OtaHealth::lastError==-1 && confirmations==1);
  assert(!OtaHealth::isPending());
  resetOta(); imageState=ESP_OTA_IMG_VALID; OtaHealth::begin(); fakeMillis=400000; OtaHealth::loop(false);
  assert(confirmations==0 && rollbacks==0);
  resetOta(); confirmError=-2; OtaHealth::loop(true); fakeMillis=120001; OtaHealth::loop(true);
  assert(OtaHealth::pending && OtaHealth::lastError==-2);
  std::cout << "PASS: batched JSON, allocation failure, delayed OTA validation, health reset, rollback fallback\n";
}
