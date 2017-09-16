#ifndef BUFFER_H
#define BUFFER_H

#include <iostream>
#include <map>
#include <string>

#include <sys/stat.h>
#include <fcntl.h>

#include "alloc.h"

using namespace std;

class buffer {
public:
  buffer(char* path, uint64_t size);
  void _copy(std::string& dst, char* src, uint64_t off, uint64_t len);
  void write(uint64_t off, uint64_t len, std::string& write);
  bool read(uint64_t off, uint64_t len, std::string& read);
  friend ostream& operator<<(ostream& out, buffer& buf);
  virtual ~buffer();
private:
  char* path;
  uint64_t size;
  int fd;
};

#endif
