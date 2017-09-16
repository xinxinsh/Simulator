#ifndef FS_H
#define FS_H

#include <iostream>
#include <map>

#include "alloc.h"
#include "buffer.h"
#include "c_snappy.h"

class extent {
public:
  extent(uint64_t len, interval_set<uint64_t>& exs, string c_type);
  friend ostream& operator<<(ostream& out, extent& e);
  void release(uint64_t toff, uint64_t tlen, interval_set<uint64_t>& erased);
  interval_set<uint64_t>& get_extents() {return exs;}
  virtual ~extent() {exs.clear();}
  uint64_t len;
  uint64_t c_len;
  interval_set<uint64_t> exs;
  string c_type;
};

ostream& operator<<(ostream& out, extent& e);

class FS {
public:
  FS(char* path, uint64_t& size, uint64_t& alloc_unit);
  void _update_extents(uint64_t off, uint64_t len, interval_set<uint64_t>& e);
  bool _copy(std::string& dst, uint64_t d_off, const char* src, uint64_t off, uint64_t len);
  int _write(interval_set<uint64_t>& e, string& write);
  void write(uint64_t off, uint64_t len, string& write);
  int _read(uint64_t off, uint64_t len, extent& e, string& read);
  bool read(uint64_t off, uint64_t len, string& read);
  void _rw_extents(uint64_t off, uint64_t len, map<uint64_t, uint64_t>& rws);
  void start(int counts);
  virtual ~FS();
  friend ostream& operator<<(ostream& out, FS& fs);
private:
  char* path;
  uint64_t size;
  uint64_t a_size;
  uint64_t c_size;
  uint64_t alloc_unit;
  buffer* buf;
  stupidallocator* alloc;
  map<uint64_t, extent> extents;
};

ostream& operator<<(ostream& out, FS& fs);
#endif
