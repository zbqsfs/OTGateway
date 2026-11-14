#pragma once
#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <vector>
class File {
  std::vector<uint8_t>* data = nullptr;
  size_t pos = 0;
public:
  File() = default;
  explicit File(std::vector<uint8_t>* p) : data(p) {}
  explicit operator bool() const { return data != nullptr; }
  size_t size() const { return data ? data->size() : 0; }
  int read() { return data && pos<data->size() ? (*data)[pos++] : -1; }
  size_t read(uint8_t* out, size_t count) {
    const auto n=std::min(count,data->size()-pos); memcpy(out,data->data()+pos,n); pos+=n; return n;
  }
  size_t write(const uint8_t* in,size_t n) { if(pos+n>data->size()) data->resize(pos+n); memcpy(data->data()+pos,in,n); pos+=n; return n; }
  size_t write(uint8_t v) { return write(&v,1); }
  bool seek(size_t n) { pos=n; return true; }
  void close() {}
};
namespace fs {
class FS {
public:
  std::map<std::string,std::vector<uint8_t>> files;
  bool exists(const char* p) { return files.count(p); }
  File open(const char* p,const char* mode) {
    if(mode[0]=='w') files[p].clear();
    if(!files.count(p)) return File();
    return File(&files[p]);
  }
};
}
