#include <iostream>

#include "FS.h"
#include "SimulatorConfig.h"

using namespace std;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << "pls specify path parameter" << std::endl;
    return -1;
  }
  char *path = argv[1];
  uint64_t size = 4*1024*1024;
  uint64_t block_size = 4*1024;
  FS* fs = new FS(path, size, block_size);
  fs->start(30);
#ifdef USE_LOG
  std::cout << *fs << std::endl;
#endif
  return 0;
}
