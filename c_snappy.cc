#include <iostream>
#include <string.h>

#include <stdint.h>
#include <snappy-c.h>

#include "compressor.h"
#include "c_snappy.h"

using namespace std;

uint64_t c_snappy::get_compressed_length(uint64_t len) {
  return snappy_max_compressed_length(len);
}

int c_snappy::get_snappy_uncompuressed_lenght(char* compressed, uint64_t& size) {
  return snappy_uncompressed_length(compressed, strlen(compressed), &size);
}

int c_snappy::compress(char* input, char* compressed) {
  uint64_t len;
  return snappy_compress(input, strlen(input), compressed, &len);
}

int c_snappy::uncompress(char* input, char* uncompressed) {
  uint64_t len;
  return snappy_uncompress(input, strlen(input), uncompressed, &len);
}
