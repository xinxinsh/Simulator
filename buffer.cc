#include <iostream>
#include <map>
#include <string.h>
#include <assert.h>
#include <cstdlib>
#include <ctime>
#include <stdlib.h>

#include <unistd.h>

#include "buffer.h"
#include "SimulatorConfig.h"

using namespace std;

buffer::buffer(char* path, uint64_t size)
  : path(path), size(size) {
  char* buf = (char*)malloc(size*sizeof(char));
  assert(buf);
  memset(buf, '-', size);
  fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd < 0) {
    std::cout << "open file " << path << " error" << std::endl;
  }
  int r = pwrite(fd, buf, size, 0);
  if (r < 0) {
    std::cout << "initial file " << path << " error" << std::endl;
  }
  delete buf;
}

buffer::~buffer() {
  close(fd);
}

void buffer::write(uint64_t off, uint64_t len, std::string& write) {
#ifdef USE_LOG
  std::cout << "write file " << off << "~" << len << std::endl;
#endif
  assert(off+len <= size);
  int r = pwrite(fd, write.c_str(), len, off);
  if (r < 0) {
    std::cout << "write file " << off << "~" << len << " " << r << endl; 
  }
  std::cout << "write file " << off << "~" << len << " r=" << r << endl; 
}

void buffer::_copy(std::string& dst, char* src, uint64_t off, uint64_t len) {
  uint64_t i=0;
  while(i<len) {
    dst[i] = *(src+off+i);
    i++;
  }
}

bool buffer::read(uint64_t off, uint64_t len, std::string& read) {
#ifdef USE_LOG
  std::cout << "read file " << off << "~" << len << std::endl;
#endif
  char* buf = (char*)malloc(len*sizeof(char));
  assert(buf);
  assert(off + len <= size);
  int r = pread(fd, buf, len, off);
  if (r < 0) {
    std::cout << "read file " << off << "~" << len << " " << r << endl; 
    return false;
  }
  _copy(read, buf, 0, r);
#ifdef USE_LOG
  std::cout << "read file " << off << "~" << len << " r=" << r << std::endl;
#endif
}

ostream& operator<<(ostream& out, buffer& buf) {
  out << buf;
  return out;
}
