#include <FileData.h>
#define SETTINGS_VALID_VALUE "stvalid"
#include "src/SettingsMigration.h"
#include <cassert>
#include <iostream>
unsigned long millis() { return 0; }
// Byte layout asserted by production code against the real Settings type.
struct Current {
  uint8_t prefix[426]{};
  struct { struct { uint8_t highTemp=15, lowTemp=10; } freezeProtection; } heating;
  uint8_t dhw[116]{};
  char validationValue[8]=SETTINGS_VALID_VALUE;
};
int main() {
  fs::FS fs;
  auto& raw=fs.files["/settings.conf"];
  raw.resize(557,0x55); raw[0]='s'; raw[427]=5;
  memcpy(raw.data()+549,SETTINGS_VALID_VALUE,8);
  const auto original=raw;
  Current current;
  FileData storage(&fs,"/settings.conf",'s',&current,sizeof(current));
  assert(readSettingsPreservingRollback(storage,fs,current)==FD_READ);
  assert(raw==original);
  assert(current.heating.freezeProtection.lowTemp==5 && current.heating.freezeProtection.highTemp==15);
  assert(memcmp(&current,original.data()+1,426)==0);
  assert(memcmp(reinterpret_cast<uint8_t*>(&current)+428,original.data()+433,124)==0);
  current.prefix[0]=42; assert(storage.updateNow()==FD_WRITE && raw==original);
  Current loaded;
  FileData next(&fs,"/settings.conf",'s',&loaded,sizeof(loaded));
  assert(readSettingsPreservingRollback(next,fs,loaded)==FD_READ && loaded.prefix[0]==42);
  uint8_t fallback[556]{};
  FileData old(&fs,"/settings.conf",'s',fallback,sizeof(fallback));
  assert(old.read()==FD_READ && memcmp(fallback,original.data()+1,556)==0);
  fs.files.erase("/settings-v2.conf"); raw[549]='x';
  Current invalid; FileData rejected(&fs,"/settings.conf",'s',&invalid,sizeof(invalid));
  assert(readSettingsPreservingRollback(rejected,fs,invalid)==FD_FILE_ERR);
  assert(!fs.exists("/settings-v2.conf"));
  std::cout << "PASS: legacy settings migration preserves original rollback file and follows new writes\n";
}
