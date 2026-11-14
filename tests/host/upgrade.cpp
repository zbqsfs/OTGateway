#include <cstdint>
#include <cstring>
#include <string>
#include <functional>
#include <cassert>
#include <iostream>
#define F(x) x
#define FPSTR(x) x
constexpr const char* L_PORTAL_OTA="OTA";
class String : public std::string {
public:
  using std::string::string;
  bool equals(const char* x) const { return *this==x; }
};
class WebServer {};
enum HTTPMethod { HTTP_POST, HTTP_GET };
enum UploadStatus { UPLOAD_FILE_START, UPLOAD_FILE_WRITE, UPLOAD_FILE_END, UPLOAD_FILE_ABORTED };
struct HTTPUpload { String name,filename; UploadStatus status; uint8_t buf[16]{}; size_t currentSize=16,totalSize=16; };
struct RequestHandler {
  virtual ~RequestHandler()=default;
  virtual bool canHandle(WebServer&,HTTPMethod,const String&) { return false; }
  virtual bool canHandle(HTTPMethod,const String&) { return false; }
  virtual bool canUpload(WebServer&,const String&) { return false; }
  virtual bool canUpload(const String&) { return false; }
  virtual bool handle(WebServer&,HTTPMethod,const String&) { return false; }
  virtual void upload(WebServer&,const String&,HTTPUpload&) {}
};
constexpr size_t UPDATE_SIZE_UNKNOWN=0;
constexpr int U_FLASH=0,U_SPIFFS=1;
struct UpdateMock {
  bool running=false, failBegin=false;
  int starts=0,aborts=0;
  bool isRunning() { return running; }
  bool begin(size_t,int) { ++starts; running=!failBegin; return running; }
  bool end(bool success) { if(!success) ++aborts; running=false; return success; }
  void clearError() {}
  bool hasError() { return failBegin; }
  const char* errorString() { return "test error"; }
  size_t write(const uint8_t*,size_t n) { return n; }
} Update;
struct Logger { template<class... T> void sinfoln(T...) {} template<class... T> void serrorln(T...) {} } Log;
#include "lib/WebServerHandlers/UpgradeHandler.h"
int main() {
  WebServer server; UpgradeHandler handler("/api/upgrade");
  bool upgrading=false; int aborted=0; bool success=false;
  handler.setBeforeUpgradeCallback([&](auto) { upgrading=true; return true; });
  handler.setUploadAbortedCallback([&] { upgrading=false; ++aborted; });
  handler.setAfterUpgradeCallback([&](const auto& fw,const auto&) { success=fw.status==UpgradeHandler::UpgradeStatus::SUCCESS; upgrading=false; });
  HTTPUpload upload; upload.name="firmware";upload.filename="firmware.bin";
  upload.status=UPLOAD_FILE_START; handler.upload(server,"/api/upgrade",upload);
  assert(upgrading && Update.running);
  upload.status=UPLOAD_FILE_ABORTED; handler.upload(server,"/api/upgrade",upload);
  assert(!upgrading && !Update.running && aborted==1 && Update.aborts==1);
  upload.status=UPLOAD_FILE_START; handler.upload(server,"/api/upgrade",upload);
  upload.status=UPLOAD_FILE_WRITE; handler.upload(server,"/api/upgrade",upload);
  upload.status=UPLOAD_FILE_END; handler.upload(server,"/api/upgrade",upload);
  handler.handle(server,HTTP_POST,"/api/upgrade"); assert(success && !upgrading);
  Update.failBegin=true;
  upload.status=UPLOAD_FILE_START; handler.upload(server,"/api/upgrade",upload); assert(upgrading);
  upload.status=UPLOAD_FILE_ABORTED; handler.upload(server,"/api/upgrade",upload);
  assert(!upgrading && aborted==2);
  Update.failBegin=false;
  upload.status=UPLOAD_FILE_START; handler.upload(server,"/api/upgrade",upload);
  assert(Update.running);
  std::cout << "PASS: disconnected OTA resumes tasks and permits retry, including after start errors\n";
}
