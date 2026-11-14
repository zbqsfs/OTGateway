#include <FileData.h>
#include "src/NetworkSettingsStorage.h"
#include <cassert>
#include <iostream>
unsigned long millis() { return 0; }
// The pre-Ethernet layout from Settings.h: 25+1+64+99+99 = 288 bytes.
struct Legacy { char hostname[25]; bool dhcp; char ip[64]; char ap[99]; char sta[99]; };
struct Current { char hostname[25]; bool dhcp; char ip[64]; char ap[99]; char sta[99]; uint8_t ethernet[8]; };
int main() {
  static_assert(sizeof(Legacy)==offsetof(Current,ethernet));
  fs::FS fs;
  Legacy old{}; strcpy(old.hostname,"opentherm"); strcpy(old.sta,"retained-wifi-config");
  FileData previous(&fs,"/network.conf",'n',&old,sizeof(old)); assert(previous.write()==FD_WRITE);
  const auto original=fs.files["/network.conf"];
  Current current{};
  FileData next(&fs,"/network.conf",'n',&current,sizeof(current));
  preserveWifiNetworkLayout(next,fs,current); next.addWithoutWipe(true);
  assert(next.read()==FD_READ); assert(next.updateNow()==FD_NO_DIF);
  assert(fs.files["/network.conf"]==original);
  strcpy(current.hostname,"new-name"); assert(next.updateNow()==FD_WRITE);
  Legacy rollback{};
  FileData fallback(&fs,"/network.conf",'n',&rollback,sizeof(rollback));
  assert(fallback.read()==FD_READ);
  assert(std::string(rollback.sta)=="retained-wifi-config" && std::string(rollback.hostname)=="new-name");
  // Existing extended records must not be shortened or reset.
  current.ethernet[0]=42;
  FileData extended(&fs,"/network.conf",'n',&current,sizeof(current)); assert(extended.write()==FD_WRITE);
  Current loaded{}; FileData reader(&fs,"/network.conf",'n',&loaded,sizeof(loaded));
  preserveWifiNetworkLayout(reader,fs,loaded); reader.addWithoutWipe(true);
  assert(reader.read()==FD_READ && loaded.ethernet[0]==42);
  assert(reader.updateNow()==FD_NO_DIF);
  std::cout << "PASS: legacy Wi-Fi migration and rollback preserve credentials; extended records retained\n";
}
