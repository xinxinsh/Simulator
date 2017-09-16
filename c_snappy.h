#ifndef C_SNAPPY_H
#define C_SNAPPY_H

#include <iostream>
#include <string.h>

#include <snappy-c.h>

#include "compressor.h"

using namespace std;

class c_snappy : public compressor {
public:
  c_snappy(std::string& type) : compressor(type) {}
  uint64_t get_compressed_length(uint64_t len);
  int get_snappy_uncompuressed_lenght(char* compressed, uint64_t& size); 
  int compress(char* input, char* compressed); 
  int uncompress(char* input, char* uncompressed);
  virtual ~c_snappy() {}
};

#endif
