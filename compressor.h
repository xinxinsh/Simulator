#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <iostream>
#include <string>

using namespace std;

class compressor {
public:
  compressor(std::string& type) : type(type) {}
  static compressor* create(string& type);
  virtual ~compressor() {}
  virtual uint64_t get_compressed_length(uint64_t len) = 0;
  virtual int get_snappy_uncompuressed_lenght(char* compressed, uint64_t& size) = 0;
  virtual int compress(char* input, char* compressed) = 0;
  virtual int uncompress(char* input, char* uncompressed) = 0;
private:
  std::string type;
};
#endif
